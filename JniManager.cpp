#include "JniManager.h"
#include <iostream>

void JniManager::setMono(MonoImage* monoImage)
{
    processRequestClass = mono_class_from_name(monoImage, "Org.Mule.Api.Dto", "ProcessRequest");
    processResponseClass = mono_class_from_name(monoImage, "Org.Mule.Api.Dto", "ProcessResponse");
    dotNetInstanceReferenceClass = mono_class_from_name(monoImage, "Org.Mule.Api.Dto", "DotNetInstanceReference");
    setTypeNameField = mono_class_get_method_from_name(processRequestClass, "set_TypeName", 1);
    setMethodNameField = mono_class_get_method_from_name(processRequestClass, "set_MethodName", 1);
    setConnectorAssemblyFilePathField = mono_class_get_method_from_name(processRequestClass, "set_ConnectorAssemblyFilePath", 1);
    setFullTrustField = mono_class_get_method_from_name(processRequestClass, "set_FullTrust", 1);
    setAssemblyFullyQualifiedNameField = mono_class_get_method_from_name(processRequestClass, "set_AssemblyFullyQualifiedName", 1);
    setLogField = mono_class_get_method_from_name(processRequestClass, "set_Log", 1);
    setDotNetInstanceReferenceField = mono_class_get_method_from_name(processRequestClass, "set_DotNetInstanceReference", 1);
    addMethodArgumentsProperty = mono_class_get_method_from_name(processRequestClass, "AddMethodArgumentProperty", 2);
    getResult = mono_class_get_method_from_name(processResponseClass, "get_Result", 0);
        
     // Get class for generic Dictionary<string, object> 
    createDictionary = mono_class_get_method_from_name(processRequestClass, "CreateDictionary", 0);
    
    dotNetInstanceCtor = mono_class_get_method_from_name(dotNetInstanceReferenceClass, ".ctor", 1);
    
    MonoObject* dictInstance = mono_runtime_invoke(createDictionary, NULL, NULL, NULL);

    MonoClass* dictionaryClass = mono_object_get_class (dictInstance);
    
    dictInstance = NULL;
    
    getItem = mono_class_get_method_from_name(dictionaryClass, "get_Item", 1);
    addItem = mono_class_get_method_from_name(dictionaryClass, "Add", 2);
    
    typeConverter = new TypeConverter();
    typeConverter->init(processRequestClass, dotNetInstanceReferenceClass);
}

void JniManager::toProcessRequest(string input, MonoObject* processRequest)
{
    Document d;
    d.Parse<0>(input.c_str());
    
    if(d.HasParseError())
    {
        string error = d.GetParseError();
        //toException(errorMessage)
        //return NULL;
        printf("SERVER PARSE ERROR: %s\n", error.c_str());
        printf("INPUT: %s\n", input.c_str());
        return;
    }

    MonoDomain* monoDomain = typeConverter->getMonoDomain();

    MonoString* assemblyFullyQualifiedName = mono_string_new(monoDomain, d["assemblyFullyQualifiedName"].GetString());
    MonoString* connectorAssemblyFilePath = mono_string_new(monoDomain, d["connectorAssemblyFilePath"].GetString());
    MonoString* methodName = mono_string_new(monoDomain, d["methodName"].GetString());
    MonoString* typeName = mono_string_new(monoDomain, d["typeName"].GetString());
    bool fullTrust = true;
    bool log = true;

    MonoObject* exception = NULL;

    void* args[1];
    args[0] = assemblyFullyQualifiedName;
    mono_runtime_invoke(setAssemblyFullyQualifiedNameField, processRequest, args, &exception);

    args[0] = connectorAssemblyFilePath;
    mono_runtime_invoke(setConnectorAssemblyFilePathField, processRequest, args, &exception);

    args[0] = methodName;
    mono_runtime_invoke(setMethodNameField, processRequest, args, &exception);
    
    args[0] = typeName;
    mono_runtime_invoke(setTypeNameField, processRequest, args, &exception);
    
    if(d["dotNetInstanceReference"].IsString())
    {
    // create instance of DotNetReference object
        MonoString* dotnetReferenceId = mono_string_new(monoDomain, d["dotNetInstanceReference"].GetString());
        MonoObject* newDotNetObject = mono_object_new(monoDomain, dotNetInstanceReferenceClass);

        args[0] = dotnetReferenceId;
        mono_runtime_invoke(dotNetInstanceCtor, newDotNetObject, args, &exception);

        args[0] = newDotNetObject;
        mono_runtime_invoke(setDotNetInstanceReferenceField, processRequest, args, &exception);
    }

    args[0] = &fullTrust;
    mono_runtime_invoke(setFullTrustField, processRequest, args, &exception);

    args[0] = &log;
    mono_runtime_invoke(setLogField, processRequest, args, &exception);

    if (exception)
    {
        const char* message = mono_string_to_utf8(mono_object_to_string(exception, NULL));
        //throwException(message);
    }
    
    /* Fill method arguments */
    if(d.HasMember("methodArguments") && d["methodArguments"].IsObject())
    {
        for (Value::ConstMemberIterator itr = d["methodArguments"].MemberBegin(); itr != d["methodArguments"].MemberEnd(); ++itr)
        {
            string keyName = string(itr->name.GetString());
            const GenericValue<UTF8<> >& value = itr->value;

            MonoObject* exc = NULL;

            void* args[] = { mono_string_new(monoDomain, keyName.c_str()), typeConverter->toMonoObject(value) };

            mono_runtime_invoke(addMethodArgumentsProperty, processRequest, args, &exc);

            if (exc)
            {
                const char* message = mono_string_to_utf8(mono_object_to_string(exc, NULL));
                return;
            }
        }
    }
}

string JniManager::toException(const char* errorMessage)
{
    StringBuffer sb;
    PrettyWriter<StringBuffer> writer(sb);
    
    writer.StartObject();
    
    writer.String("exception");
    writer.String(errorMessage);
    
    writer.EndObject();
    
    return sb.GetString();
}

string JniManager::toResponse(MonoObject* monoObject)
{
    assert(monoObject);

    MonoObject* exc = NULL;

    MonoObject* result = mono_runtime_invoke(getResult, monoObject, NULL, &exc);

    if (exc)
    {
        const char* message = mono_string_to_utf8(mono_object_to_string(exc, NULL));
        return toException(message);
    }
    
    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
    
    // set Mule Message properties
    writer.StartObject();

    if (result != NULL)
    {
        writer.String("payload");
        typeConverter->toJson(result, writer);
    }
    else
    {
        
    }

    writer.EndObject();
    return sb.GetString();
}