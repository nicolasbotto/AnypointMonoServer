#include "Server.h"
#include "ThreadPool.h"

static JniManager* jniManager;
static MonoImage* monoImage;
static MonoImage* dtoMonoImage;
static MonoMethod* processMethod;
static MonoMethod* newProcessRequest;
static uint32_t routerInstance_handle;
static MonoClass* processRequestClass;
static ThreadPool* threadPool;
//static bool isInitialized = false;
//pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void logToJava(MonoString* message)
{
    char* c_message = mono_string_to_utf8(message);
    //jniManager->log(c_message);
    //printf("Log: %s\n", c_message);
    
    mono_free(c_message);
}

void instrumentToJava(MonoString* message, int id)
{
    char* c_message = mono_string_to_utf8(message);
    //jniManager->instrument(c_message, id);
    //printf("Log: %s\n", c_message);
    
    mono_free(c_message);
}

Server::Server(const char* socket, const char* path)
{
    socketName = socket;
    InitMono(path);
}

void Server::InitMono(const char* path)
{
    // initialize the root domain which will hold corlib and will always be alive
    try
    {
        jniManager = new JniManager();
        
        mono_config_parse(NULL);

        MonoDomain* monoDomain = mono_jit_init_version("jni Root Domain", "v4.0.30319");
        
        // mono callbacks
        //mono_add_internal_call("Org.Mule.Api.Routing.Router::LogToMono(string)", (void*)logToJava);
        //mono_add_internal_call("Org.Mule.Api.Routing.Router::InstrumentToMono(string,int)", (void*)instrumentToJava);
        
        string workingFolder(path);
        workingFolder.append("/Org.Mule.Api.Routing.dll");
        
        string dtoWorkingFolder(path);
        dtoWorkingFolder.append("/Org.Mule.Api.Dto.dll");
                
        MonoAssembly* assembly = mono_domain_assembly_open(monoDomain, workingFolder.c_str());
        
        monoImage = mono_assembly_get_image(assembly);
        
        MonoAssembly* dtoAssembly = mono_domain_assembly_open(monoDomain, dtoWorkingFolder.c_str());
        dtoMonoImage = mono_assembly_get_image(dtoAssembly);
        
        MonoClass* routerClass = mono_class_from_name(monoImage, "Org.Mule.Api.Routing", "Router");

        MonoObject* router = mono_object_new(monoDomain, routerClass);
        mono_runtime_object_init(router);

        routerInstance_handle = mono_gchandle_new(router, 1);

        processMethod = mono_class_get_method_from_name(routerClass,"Process", 1);
        
        newProcessRequest = mono_class_get_method_from_name(routerClass,"CreateProcessRequest", 0);

        jniManager->setMono(dtoMonoImage);

        processRequestClass = mono_class_from_name(dtoMonoImage, "Org.Mule.Api.Dto", "ProcessRequest");
        
        threadPool = new ThreadPool(10);
        threadPool->initialize_threadpool();
        //jniManager->setRouter(obj);
        
        //isInitialized = true;
    }
    catch (char* errorMessage)
    {
        printf("%s\n", errorMessage);
        //jniManager->throwException(errorMessage);
    }
}

void Server::Stop()
{
    started = false;
}

void Server::HandleRequest(int data_socket)
{
     //printf("%s\n", "Connected");
    /* Wait for next data packet. */
    char data[1048576] = {};
    int ret = read(data_socket, data, sizeof(data));
    if (ret == -1) {
        perror("Error reading server buffer.");
    }
    
    if(ret > 0)
    {
        // Mono execution
        MonoObject* exception = NULL;

        MonoObject *routerInstance = mono_gchandle_get_target (routerInstance_handle);

        MonoDomain* monoDomain = mono_get_root_domain();
        mono_thread_attach(monoDomain);

        MonoObject* processRequest = mono_object_new(monoDomain, processRequestClass);
        mono_runtime_object_init(processRequest);

        jniManager->toProcessRequest(data, processRequest);

        void* args[1];
        args[0] = processRequest;

        MonoObject* response = mono_runtime_invoke(processMethod, routerInstance, args, &exception);

        // check for any thrown exception
        if (exception)
        {
            mono_print_unhandled_exception(exception);
            const char* message = mono_string_to_utf8(mono_object_to_string(exception, NULL));

            string exceptionJson = jniManager->toException(message);

            ret = write(data_socket, exceptionJson.c_str(), strlen(exceptionJson.c_str()));
            if (ret == -1) 
            {
                string errorMessage = "Server error sending exception message: ";
                errorMessage.append(strerror(errno));
                printf("%s", errorMessage.c_str());
            }
        }
        else
        {
            string result = jniManager->toResponse(response);

            /* Send result. */
            //printf("%s", result.c_str());
            ret = write(data_socket, result.c_str(), strlen(result.c_str()));

            if (ret == -1) 
            {
                string errorMessage = "Server error sending response message: ";
                errorMessage.append(strerror(errno));
                printf("%s", errorMessage.c_str());
            }
        }
    }
    
        // Close accepted socket
    close(data_socket);
}

void Server::Start()
{
    int ret;
   
    /* Create local socket. */
    connection_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (connection_socket == -1) {
        perror("Error creating socket.");
    }

    /*
     * For portability clear the whole structure, since some
     * implementations have additional (nonstandard) fields in
     * the structure.
     */

    memset(&name, 0, sizeof(struct sockaddr_un));

    /* Bind socket to socket name. */

    name.sun_family = AF_UNIX;
    strncpy(name.sun_path, socketName, sizeof(name.sun_path) - 1);

    /*
    * In case the program exited inadvertently on the last run,
    * remove the socket.
    */

    unlink(name.sun_path);
    
    ret = bind(connection_socket, (const struct sockaddr *) &name,
               sizeof(struct sockaddr_un));
    if (ret == -1) {
        perror("Error binding socket.");
    }

    /*
     * Prepare for accepting connections. The backlog size is set
     * to 1000. So while one request is being processed other requests
     * can be waiting.
     */

    ret = listen(connection_socket, 1000);
    if (ret == -1) {
        perror("Error start listening.");
    }
    
    started = true;
    
    cout << "Server started" << endl;

    /* This is the main loop for handling connections. */
    while(started)
    {
        /* Wait for incoming connection. */
        
        //printf("%s\n", "Waiting incoming connection");
        int data_socket = accept(connection_socket, NULL, NULL);
        if (ret == -1) {
            perror("Error accepting connection.");
        }

        // handle requests in thread pool
        
        Task* t = new Task();
        t->setCallBack(&Server::HandleRequest);
        t->setCallBackArg(data_socket);
        t->setServerInstance(this);
        threadPool->add_task(t);
        //HandleRequest(data_socket);
    }
    
    // Close Connection socket
    close(connection_socket);
    threadPool->destroy_threadpool();
    Server::Stop();
}