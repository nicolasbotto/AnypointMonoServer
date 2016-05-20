#pragma once
#include <mono/metadata/mono-config.h>
#include <mono/metadata/threads.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/mono-gc.h>
#include <mono/metadata/environment.h>
#include <mono/jit/jit.h>
#include <cassert>
#include <vector>
#include <string>
#include <exception>
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"

typedef unsigned char byte;
using namespace rapidjson;
using namespace std;

class TypeConverter
{
private:
    MonoClass* dotNetInstanceReferenceClass;
    MonoMethod* getDotNetInstanceId;
    MonoMethod* getItem;
    MonoMethod* addItem;
    MonoMethod* createDictionary;
    MonoMethod* dotNetInstanceCtor;

public:
    void init(MonoClass*, MonoClass*);
    MonoDomain* getMonoDomain();
    MonoObject* toMonoObject(const GenericValue<UTF8<> >&);
    
    template <typename Writer>
    void toJson(MonoObject* systemObject, Writer &writer)
    {
        if (systemObject == NULL)
        {
            writer.StartObject();
            writer.String("Jni_Type");
            writer.String("NULL");
            writer.String("Jni_Value");
            writer.Null();
            writer.EndObject();
            return;
        }

        string className = mono_class_get_name(mono_object_get_class(systemObject));

        writer.StartObject();
        
        if(className == "DotNetInstanceReference")
        {
            MonoObject* exc = NULL;
            MonoObject* monoDotNetInstanceId = mono_runtime_invoke(getDotNetInstanceId, systemObject, NULL, &exc);
            
            char* dotNetInstanceId = mono_string_to_utf8((MonoString*)monoDotNetInstanceId);
            
            if (exc)
            {
                const char* message = mono_string_to_utf8(mono_object_to_string(exc, NULL));
                //return NULL;
            }

            writer.String("Jni_Type");
            writer.String("DotNetInstanceReference");
            writer.String("Jni_Value");
            writer.String(dotNetInstanceId);
        }

        if (className == "Int32")
        {
            int value = *(int*)mono_object_unbox(systemObject);

            writer.String("Jni_Type");
            writer.String("java.lang.Integer");
            writer.String("Jni_Value");
            writer.Int(value);
        }

        if (className == "Double")
        {
            double value = *(double*)mono_object_unbox(systemObject);

            writer.String("Jni_Type");
            writer.String("java.lang.Double");
            writer.String("Jni_Value");
            writer.Double(value);
        }

        if (className == "Boolean")
        {
            bool value = *(bool*)mono_object_unbox(systemObject);

            writer.String("Jni_Type");
            writer.String("java.lang.Boolean");
            writer.String("Jni_Value");
            writer.Bool(value);
        }

        if (className == "Char")
        {
            char value = *(char*)mono_object_unbox(systemObject);

            writer.String("Jni_Type");
            writer.String("java.lang.Character");
            writer.String("Jni_Value");
            writer.Int(value);
        }

        if (className == "Int16")
        {
            short value = *(short*)mono_object_unbox(systemObject);

            writer.String("Jni_Type");
            writer.String("java.lang.Short");
            writer.String("Jni_Value");
            writer.Int(value);
        }

        if (className == "Single")
        {
            float value = *(float*)mono_object_unbox(systemObject);

            writer.String("Jni_Type");
            writer.String("java.lang.Float");
            writer.String("Jni_Value");
            writer.Double(value);
        }

        if (className == "Int64")
        {
            long long value = *(long long*)mono_object_unbox(systemObject);

            writer.String("Jni_Type");
            writer.String("java.lang.Long");
            writer.String("Jni_Value");
            writer.Int64(value);
        }

        if (className == "Byte")
        {
            byte value = *(byte*)mono_object_unbox(systemObject);

            writer.String("Jni_Type");
            writer.String("java.lang.Byte");
            writer.String("Jni_Value");
            writer.Uint(value);
        }

        if (className == "String")
        {
            char* cString = mono_string_to_utf8((MonoString*)systemObject);

            writer.String("Jni_Type");
            writer.String("java.lang.String");
            writer.String("Jni_Value");
            writer.String(cString);

            mono_free(cString);
        }

        // Arrays
        if (className == "String[]")
        {
            writer.String("Jni_Type");
            writer.String("[Ljava.lang.String;");
            writer.String("Jni_Value");

            MonoArray *data = (MonoArray*)systemObject;
            uintptr_t intsSize = mono_array_length(data);

            writer.StartArray();
            for (int i = 0; i < intsSize; i++)
            {
                MonoObject* monoString = (MonoObject*)mono_array_get(data, MonoString*, i);
        
                MonoObject* exc = NULL;
                char* cString =  mono_string_to_utf8(mono_object_to_string(monoString, &exc));
                writer.String(cString);
                
                mono_free(cString);
            }

            writer.EndArray();
        }
        
        if (className == "Byte[]")
        {
            writer.String("Jni_Type");
            writer.String("[B");
            writer.String("Jni_Value");

            MonoArray *data = (MonoArray*)systemObject;
            uintptr_t intsSize = mono_array_length(data);

            writer.StartArray();
            for (int i = 0; i < intsSize; i++)
            {
                writer.Uint(mono_array_get(data, uint8_t, i));
            }

            writer.EndArray();
        }
        
        if (className == "Int32[]")
        {
            writer.String("Jni_Type");
            writer.String("[I");
            writer.String("Jni_Value");

            MonoArray *data = (MonoArray*)systemObject;
            uintptr_t intsSize = mono_array_length(data);

            writer.StartArray();
            for (int i = 0; i < intsSize; i++)
            {
                writer.Int(mono_array_get(data, int, i));
            }

            writer.EndArray();
        }
        
        if (className == "Int16[]")
        {
            writer.String("Jni_Type");
            writer.String("[S");
            writer.String("Jni_Value");

            MonoArray *data = (MonoArray*)systemObject;
            uintptr_t intsSize = mono_array_length(data);

            writer.StartArray();
            for (int i = 0; i < intsSize; i++)
            {
                writer.Int(mono_array_get(data, short, i));
            }

            writer.EndArray();
        }
        
        if (className == "Int64[]")
        {
            writer.String("Jni_Type");
            writer.String("[J");
            writer.String("Jni_Value");

            MonoArray *data = (MonoArray*)systemObject;
            uintptr_t intsSize = mono_array_length(data);

            writer.StartArray();
            for (int i = 0; i < intsSize; i++)
            {
                writer.Int64(mono_array_get(data, long, i));
            }

            writer.EndArray();
        }
        
        if (className == "Boolean[]")
        {
            writer.String("Jni_Type");
            writer.String("[Z");
            writer.String("Jni_Value");

            MonoArray *data = (MonoArray*)systemObject;
            uintptr_t intsSize = mono_array_length(data);

            writer.StartArray();
            for (int i = 0; i < intsSize; i++)
            {
                writer.Bool(mono_array_get(data, bool, i));
            }

            writer.EndArray();
        }
        
        if (className == "Double[]")
        {
            writer.String("Jni_Type");
            writer.String("[D");
            writer.String("Jni_Value");

            MonoArray *data = (MonoArray*)systemObject;
            uintptr_t intsSize = mono_array_length(data);

            writer.StartArray();
            for (int i = 0; i < intsSize; i++)
            {
                writer.Double(mono_array_get(data, double, i));
            }

            writer.EndArray();
        }
        
        if (className == "Single[]")
        {
            writer.String("Jni_Type");
            writer.String("[F");
            writer.String("Jni_Value");

            MonoArray *data = (MonoArray*)systemObject;
            uintptr_t intsSize = mono_array_length(data);

            writer.StartArray();
            for (int i = 0; i < intsSize; i++)
            {
                writer.Double(mono_array_get(data, float, i));
            }

            writer.EndArray();
        }
        
        if (className == "Char[]")
        {
            writer.String("Jni_Type");
            writer.String("[C");
            writer.String("Jni_Value");

            MonoArray *data = (MonoArray*)systemObject;
            uintptr_t intsSize = mono_array_length(data);

            writer.StartArray();
            for (int i = 0; i < intsSize; i++)
            {
                writer.Int(mono_array_get(data, mono_unichar2, i));
            }

            writer.EndArray();
        }
             
        writer.EndObject();
    }
    
    template <typename Writer>
    void convertToJsonMap(MonoObject* obj, MonoMethod* sizeMethod, MonoMethod* getMethodValue, MonoMethod* getKey, Writer &writer)
    {
        assert(obj);
        assert(sizeMethod);
        assert(getKey);
        assert(getMethodValue);

        if (obj == NULL)
        {
            writer.StartObject();
            writer.EndObject();
            return;
        }

        MonoObject* exc = NULL;

        MonoObject* resultSize = mono_runtime_invoke(sizeMethod, obj, NULL, &exc);

        if(exc != NULL)
        {
            writer.StartObject();
            writer.EndObject();
            return;
        }

        writer.StartObject();
        writer.String("Jni_Type");
        writer.String("Map");
        writer.String("Jni_Value");
        
        writer.StartObject();
        int size = *(int*)mono_object_unbox(resultSize);

        for(int i=0; i<size; i++)
        {
            void* args[1];
            args[0] = &i;
            MonoObject* resultKey = mono_runtime_invoke(getKey, obj, args, &exc);

            args[0] = (MonoString*)resultKey;

            MonoObject* resultValue = mono_runtime_invoke(getMethodValue, obj, args, &exc);

            char* cString = mono_string_to_utf8((MonoString*)resultKey);

            writer.String(cString);
            mono_free(cString);

            toJson(resultValue, writer);
        }
        
        writer.EndObject(); // java_value object
        writer.EndObject(); // map object
    }
};