#ifndef ___BLUETOOTH_FD_H___
#define ___BLUETOOTH_FD_H___

#include <nan.h>
#include <node.h>

class BluetoothFd : public Nan::ObjectWrap {
   public:
    static NAN_MODULE_INIT(Init);
    static NAN_METHOD(New);
    static NAN_METHOD(Start);
    static NAN_METHOD(Stop);
    static NAN_METHOD(Write);
    static NAN_METHOD(Close);

   private:
    BluetoothFd(int fd, const v8::Local<v8::Function>& readCallback);
    ~BluetoothFd();

    void start();
    void stop();

    int write_(char* data, int length);
    bool close_();

    void poll(int status);

    void emitErrnoError();

    static void PollCallback(uv_poll_t* handle, int status, int events);

   private:
    Nan::Persistent<v8::Object> This;

    int _fd;
    Nan::Callback _readCallback;

    bool isReading;
    uv_poll_t _pollHandle;

    static Nan::Persistent<v8::FunctionTemplate> constructor_template;
};

#endif