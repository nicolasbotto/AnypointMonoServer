#include "TypeConverter.h"
#include "b64.h"

using namespace std;

void TypeConverter::init(MonoClass* processRequestClass, MonoClass* dotNetInstanceReference)
{
    // Get class for generic Dictionary<string, object> 
    dotNetInstanceReferenceClass = dotNetInstanceReference;
    createDictionary = mono_class_get_method_from_name(processRequestClass, "CreateDictionary", 0);
    getDotNetInstanceId = mono_class_get_method_from_name(dotNetInstanceReferenceClass, "get_DotNetInstanceId", 0);
    dotNetInstanceCtor = mono_class_get_method_from_name(dotNetInstanceReferenceClass, ".ctor", 1);
        
    MonoObject* dictInstance = mono_runtime_invoke(createDictionary, NULL, NULL, NULL);

    MonoClass* dictionaryClass = mono_object_get_class (dictInstance);
    
    dictInstance = NULL;
    
    getItem = mono_class_get_method_from_name(dictionaryClass, "get_Item", 1);
    addItem = mono_class_get_method_from_name(dictionaryClass, "Add", 2);
}

MonoDomain* TypeConverter::getMonoDomain()
{
    MonoDomain* monoDomain = mono_get_root_domain();
    mono_thread_attach(monoDomain);
    return monoDomain;
}

MonoObject* TypeConverter::toMonoObject(const GenericValue<UTF8<> >& jsonValue)
{
    MonoDomain* monoDomain = getMonoDomain();

    MonoObject* result = NULL;
    
    if(!jsonValue.HasMember("Jni_Type"))
    {
        return result;
    }
    
    string className = jsonValue["Jni_Type"].GetString();
    
    if(className == "DotNetInstanceReference")
    {
        MonoString* dotnetReferenceId = mono_string_new(monoDomain, jsonValue["Jni_Value"].GetString());
        MonoObject* newDotNetObject = mono_object_new(monoDomain, dotNetInstanceReferenceClass);
        MonoObject* exception = NULL;
        
        void* args[1];
        args[0] = dotnetReferenceId;
        mono_runtime_invoke(dotNetInstanceCtor, newDotNetObject, args, &exception);

        result = newDotNetObject;
    }
    
    if (className == "Int32")
    {
        int value = jsonValue["Jni_Value"].GetInt();
        result = mono_value_box(monoDomain, mono_get_int32_class(), &value);
    }

    if (className == "String")
    {
        string value = jsonValue["Jni_Value"].GetString();
        result = (MonoObject*)mono_string_new(monoDomain, value.c_str());
    }

    if (className == "Boolean")
    {
        bool value = jsonValue["Jni_Value"].GetBool();
        result = (MonoObject*)mono_value_box(monoDomain, mono_get_boolean_class(), &value);
    }

    if (className == "Char")
    {
        char value = (char)jsonValue["Jni_Value"].GetString()[0];
        result = (MonoObject*)mono_value_box(monoDomain, mono_get_char_class(), &value);
    }

    if (className == "Int64")
    {
        long value = jsonValue["Jni_Value"].GetInt64();
        result = (MonoObject*)mono_value_box(monoDomain, mono_get_int64_class(), &value);
    }

    if (className == "Int16")
    {
        short value = (short)jsonValue["Jni_Value"].GetInt();
        result = (MonoObject*)mono_value_box(monoDomain, mono_get_int16_class(), &value);
    }

    if (className =="Byte")
    {
        string encoded = jsonValue["Jni_Value"].GetString();
        string decoded = base64_decode(encoded);
        result = (MonoObject*)mono_value_box(monoDomain, mono_get_byte_class(), &decoded.at(0));
    }

    if (className == "Double" )
    {
        double value = jsonValue["Jni_Value"].GetDouble();
        result = (MonoObject*)mono_value_box(monoDomain, mono_get_double_class(), &value);
    }

    if (className == "Single" )
    {
        float value = (float)jsonValue["Jni_Value"].GetDouble();
        result = (MonoObject*)mono_value_box(monoDomain, mono_get_single_class(), &value);
    }

    if (className == "Map" )
    {
        /* Fill method arguments */
        MonoObject* dictionaryInstance = mono_runtime_invoke(createDictionary, NULL, NULL, NULL);

        if(jsonValue.HasMember("Jni_Value"))
        {
            for (Value::ConstMemberIterator itr = jsonValue["Jni_Value"].MemberBegin(); itr != jsonValue["Jni_Value"].MemberEnd(); ++itr)
            {
                string keyName = string(itr->name.GetString());
                const GenericValue<UTF8<> >& mapValue = itr->value;

                MonoObject* exc = NULL;

                void* args[] = { mono_string_new(monoDomain, keyName.c_str()), toMonoObject(mapValue) };

                mono_runtime_invoke(addItem, dictionaryInstance, args, &exc);

                if (exc)
                {
                    char* message = mono_string_to_utf8(mono_object_to_string(exc, NULL));
                    perror(message);
                    mono_free(message);
                    // throw exception
                }
            }
        }

        result = dictionaryInstance;
    }
    
    if (className == "Int32[]")
    {
        const Value& arr = jsonValue["Jni_Value"];

        if(arr.IsArray())
        {
            MonoArray *data = mono_array_new (monoDomain, mono_get_int32_class(), arr.Size());

            for (SizeType i = 0; i < arr.Size(); i++)
            {
                mono_array_set(data, int, i, arr[i].GetInt());
            }

            result = (MonoObject*)data;
        }
    }
    
    if (className == "Byte[]")
    {
        // java json serializer uses base64 encoding
        const Value& input = jsonValue["Jni_Value"];
        
        string jsonData = input.GetString();
        
        string decoded = base64_decode(jsonData);
        
        int size = decoded.size();
       
        MonoArray *data = mono_array_new (monoDomain, mono_get_byte_class(), size);

        for (SizeType i = 0; i < size; i++)
        {
            mono_array_set(data, uint8_t, i, decoded.at(i));
        }

        result = (MonoObject*)data;
        
    }
    
    if (className == "Char[]")
    {
        const Value& input = jsonValue["Jni_Value"];
        
        string jsonData = input.GetString();
        
        int size = jsonData.size();
       
        MonoArray *data = mono_array_new (monoDomain, mono_get_char_class(), size);

        for (SizeType i = 0; i < size; i++)
        {
            mono_array_set(data, mono_unichar2, i, jsonData.at(i));
        }

        result = (MonoObject*)data;
    }
    
    if (className == "Double[]")
    {
        const Value& arr = jsonValue["Jni_Value"];

        if(arr.IsArray())
        {
            MonoArray *data = mono_array_new (monoDomain, mono_get_double_class(), arr.Size());

            for (SizeType i = 0; i < arr.Size(); i++)
            {
                mono_array_set(data, double, i, arr[i].GetDouble());
            }

            result = (MonoObject*)data;
        }
    }
    
    if (className == "Bool[]")
    {
        const Value& arr = jsonValue["Jni_Value"];

        if(arr.IsArray())
        {
            MonoArray *data = mono_array_new (monoDomain, mono_get_boolean_class(), arr.Size());

            for (SizeType i = 0; i < arr.Size(); i++)
            {
                mono_array_set(data, bool, i, arr[i].GetBool());
            }

            result = (MonoObject*)data;
        }
    }
    
    if (className == "Short[]")
    {
        const Value& arr = jsonValue["Jni_Value"];

        if(arr.IsArray())
        {
            MonoArray *data = mono_array_new (monoDomain, mono_get_int16_class(), arr.Size());

            for (SizeType i = 0; i < arr.Size(); i++)
            {
                mono_array_set(data, short, i, (short)arr[i].GetInt());
            }

            result = (MonoObject*)data;
        }
    }
    
    if (className == "Long[]")
    {
        const Value& arr = jsonValue["Jni_Value"];

        if(arr.IsArray())
        {
            MonoArray *data = mono_array_new (monoDomain, mono_get_int64_class(), arr.Size());

            for (SizeType i = 0; i < arr.Size(); i++)
            {
                mono_array_set(data, long, i, arr[i].GetInt64());
            }

            result = (MonoObject*)data;
        }
    }
    
    if (className == "Float[]")
    {
        const Value& arr = jsonValue["Jni_Value"];

        if(arr.IsArray())
        {
            MonoArray *data = mono_array_new (monoDomain, mono_get_single_class(), arr.Size());

            for (SizeType i = 0; i < arr.Size(); i++)
            {
                mono_array_set(data, float, i, (float)arr[i].GetDouble());
            }

            result = (MonoObject*)data;
        }
    }
    
    if (className == "String[]")
    {
        const Value& arr = jsonValue["Jni_Value"];

        if(arr.IsArray())
        {
            MonoArray *data = mono_array_new (monoDomain, mono_get_string_class(), arr.Size());

            for (SizeType i = 0; i < arr.Size(); i++)
            {
                string tmp = arr[i].GetString();
                MonoString* value = mono_string_new(monoDomain, tmp.c_str());
                mono_array_setref(data, i, value);
            }

            result = (MonoObject*)data;
        }
    }
    
    return result;
}