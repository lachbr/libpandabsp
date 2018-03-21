## Panda Plus Library
Panda Plus is a C++ extension library to the Panda3D game engine. It provides numerous tools to assist the C++ Panda developer in making games.
***
#### Distributed Object System
Panda3D's Distributed Object System was created for Python use only. Panda Plus provides a complete C++ implementation of the Python DO system. It has support for the Astron server and the CMU server.

Overall, it is very similar to the Python implementation, with a few key differences:
* Due to the lack of introspection in C++, the system cannot automatically determine which class in the DC file corresponds to which C++ class, and which field in the DC file corresponds to which C++ function.
    * This means that you have to explicity declare this information in your code, which Panda Plus provides helpful macros to do so.
    * Here's an example:
```c++
// Client side implementation of DistributedObject.
class DistributedObject : public DistributedObjectBase {
  DClassDecl(DistributedObject, DistributedObjectBase);

public:
  void set_name(const string &name);
  static void set_name_field(DCPacker &packer, void *data);
  
  void do_something();
  static void do_something_field(DCPacker &packer, void *data);
  
  InitTypeStart();
  DCFieldDefStart();
  
  // fieldname MUST correspond to the name of the field in the DC file.
  //
  //        fieldname    setterfunc      getterfunc
  DCFieldDef(set_name, set_name_field, get_name_field); // This is an example of a "required broadcast ram"
                                                        // where there has to be a setter and getter.
  
  DCFieldDef(do_something, do_something_field, NULL); // This is an example of a "broadcast" event message.
                                                      // No getter is required.
  
  DCFieldDef(request_something, NULL, NULL); // This is an example of a "clsend airecv".
                                             // Only the AI receives this field, so we
                                             // don't need a setter or a getter.
                                             // Note, the AI version of this object would
                                             // define this field having a setter since they
                                             // are receiving it.
  
  DCFieldDefEnd();
  InitTypeEnd();
};
```
#### Sending Updates

From the client:
```c++
// Prepare the update.
BeginCLUpdate("set_name");
// Pack in any arguments that are needed by this field.
//    datatype, value
AddArg(string, "Brian");
// Now, send it out.
SendCLUpdate("set_name");
```
From the AI/UD:
```c++
// Prepare the update.

// In this example, the update is being sent to the client on channel 100000006.
BeginAIUpdate_CH("login_accepted", 100000006);

// You can also send the update to avatar ids, account ids, or everyone:
// BeginAIUpdate("login_accepted");
// BeginAIUpdate_AVID("login_accepted", 110000001);
// BeginAIUpdate_ACC("login_accepted", 42000002);

// Packing in arguments is the same on the AI/UD:
// AddArg(uint, 1234567890);

// Now, send it out.
SendAIUpdate("login_accepted");
```

#### Receiving Updates
Header file:
```c++
class MyDistributedObject : public DistributedObject {
...
  void set_name(const string &name);
  static void set_name_field(DCPacker &packer, void *data);
...
```
Source file:
```c++
...
void MyDistributedObject::
set_name(const string &name) {
  _name = name;
}

void MyDistributedObject::
set_name_field(DCPacker &packer, void *data) {
  MyDistributedObject *obj = (MyDistributedObject *)data;
  // Unpack the string that was packed in by the sender.
  string name = packer.unpack_string();
  // Hand it over to the object.
  obj->set_name(name);
}
...
```