#pragma once
#include <map>
#include "TypeConverter.h"
#include <iostream>

using namespace std;
using namespace rapidjson;

class JniManager
{
private:
    TypeConverter* typeConverter;
   
    // Mono Methods
    MonoMethod* processMethod;
    MonoMethod* setConnectorAssemblyFilePathField;
    MonoMethod* setMethodNameField;
    MonoMethod* setTypeNameField;
    MonoMethod* setFullTrustField;
    MonoMethod* setAssemblyFullyQualifiedNameField;
    MonoMethod* setLogField;
    MonoMethod* setDotNetInstanceReferenceField;
    MonoMethod* addMethodArgumentsProperty;
    MonoMethod* getResult;
    MonoMethod* newDotNetInstanceReference;
    MonoMethod* dotNetInstanceCtor;

    MonoClass* processRequestClass;
    MonoClass* processResponseClass;
    MonoClass* dotNetInstanceReferenceClass;
    MonoMethod* getItem;
    MonoMethod* addItem;
    MonoMethod* createDictionary;

public:
    void setMono(MonoImage*);
    string toResponse(MonoObject*);
    void toProcessRequest(string, MonoObject*);
    string toException(const char*);
};

