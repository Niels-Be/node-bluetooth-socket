#include "bluetooth-fd.h"

#include <errno.h>
#include <nan.h>
#include <node_buffer.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using namespace v8;

Nan::Persistent<FunctionTemplate> BluetoothFd::constructor_template;

BluetoothFd::BluetoothFd(int fd, const Local<Function>& readCallback)
    : Nan::ObjectWrap(), _fd(fd), _readCallback(readCallback), isReading(false) {
    uv_poll_init(uv_default_loop(), &this->_pollHandle, this->_fd);

    this->_pollHandle.data = this;
}

BluetoothFd::~BluetoothFd() {
    stop();
    uv_close((uv_handle_t*)&this->_pollHandle, nullptr);

    if (this->_fd != -1) {
        close(this->_fd);
    }
}

void BluetoothFd::start() {
    if (!this->isReading) {
        this->isReading = true;
        uv_poll_start(&this->_pollHandle, UV_READABLE | UV_DISCONNECT, BluetoothFd::PollCallback);
    }
}

void BluetoothFd::poll(int status) {
    Nan::HandleScope scope;

    if(status < 0) {
        Local<Value> argv[2] = {Nan::New<Number>(status), Nan::Null()};
        Nan::Call(this->_readCallback, Nan::GetCurrentContext()->Global(), 2, argv);
        return;
    }

    int length = 0;
    char data[1024];

    length = read(this->_fd, data, sizeof(data));

    if (length > 0) {
        Local<Value> argv[2] = {Nan::New<Number>(0), Nan::CopyBuffer(data, length).ToLocalChecked()};

        Nan::Call(this->_readCallback, Nan::GetCurrentContext()->Global(), 2, argv);
    } else {
        // NOTE: errno = EAGAIN will be set if the remote closed the socket
        // socket is probably broken so stop polling
        this->stop();

        Local<Value> argv[2] = {Nan::New<Number>(uv_translate_sys_error(errno)), Nan::Null()};
        Nan::Call(this->_readCallback, Nan::GetCurrentContext()->Global(), 2, argv);
    }
}

void BluetoothFd::stop() {
    if (this->isReading) {
        this->isReading = false;
        uv_poll_stop(&this->_pollHandle);
    }
}

int BluetoothFd::write_(char* data, int length) { return write(this->_fd, data, length); }

bool BluetoothFd::close_() {
    this->stop();
    // if(shutdown(this->_fd, SHUT_RDWR) != 0)
    //  return false;
    if(this->_fd != -1) {
        if (close(this->_fd) != 0) return false;
        this->_fd = -1;
    }
    return true;
}

NAN_METHOD(BluetoothFd::New) {
    Nan::HandleScope scope;

    if (info.Length() != 2) {
        return Nan::ThrowTypeError("usage: BluetoothFd(fd, readCallback)");
    }

    Local<Value> arg0 = info[0];
    if (!(arg0->IsInt32() || arg0->IsUint32())) {
        return Nan::ThrowTypeError("usage: BluetoothFd(fd, readCallback)");
    }
    int fd = Nan::To<int32_t>(arg0).FromJust();
    if (fd < 0) {
        return Nan::ThrowTypeError("usage: BluetoothFd(fd, readCallback)");
    }

    Local<Value> arg1 = info[1];
    if (!arg1->IsFunction()) {
        return Nan::ThrowTypeError("usage: BluetoothFd(fd, readCallback)");
    }

    BluetoothFd* p = new BluetoothFd(fd, arg1.As<Function>());
    p->Wrap(info.This());
    // p->This.Reset(info.This());
    info.GetReturnValue().Set(info.This());
}

NAN_METHOD(BluetoothFd::Start) {
    Nan::HandleScope scope;

    BluetoothFd* p = Nan::ObjectWrap::Unwrap<BluetoothFd>(info.Holder());

    p->start();

    info.GetReturnValue().SetUndefined();
}

NAN_METHOD(BluetoothFd::Stop) {
    Nan::HandleScope scope;

    BluetoothFd* p = Nan::ObjectWrap::Unwrap<BluetoothFd>(info.Holder());

    p->stop();

    info.GetReturnValue().SetUndefined();
}

NAN_METHOD(BluetoothFd::Write) {
    BluetoothFd* p = Nan::ObjectWrap::Unwrap<BluetoothFd>(info.Holder());

    if (info.Length() > 0) {
        Local<Value> arg0 = info[0];
        if (arg0->IsObject()) {
            int res = p->write_(node::Buffer::Data(arg0), node::Buffer::Length(arg0));
            if (res > 0) {
                info.GetReturnValue().Set(0);
            } else {
                info.GetReturnValue().Set(errno);
            }

        } else {
            return Nan::ThrowTypeError("Can only write Buffers");
        }
    } else {
        return Nan::ThrowTypeError("usage: write(buffer)");
    }
}

NAN_METHOD(BluetoothFd::Close) {
    BluetoothFd* p = Nan::ObjectWrap::Unwrap<BluetoothFd>(info.Holder());
    if (!p->close_()) {
        return Nan::ThrowError(Nan::ErrnoException(errno));
    }

    info.GetReturnValue().SetUndefined();
}

void BluetoothFd::PollCallback(uv_poll_t* handle, int status, int events) {
    BluetoothFd* p = (BluetoothFd*)handle->data;

    p->poll(status);
}

NAN_MODULE_INIT(BluetoothFd::Init) {
    Nan::HandleScope scope;

    Local<FunctionTemplate> tmpl = Nan::New<FunctionTemplate>(New);

    tmpl->SetClassName(Nan::New("BluetoothFd").ToLocalChecked());
    tmpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tmpl, "start", Start);
    Nan::SetPrototypeMethod(tmpl, "stop", Stop);
    Nan::SetPrototypeMethod(tmpl, "write", Write);
    Nan::SetPrototypeMethod(tmpl, "close", Close);

    constructor_template.Reset(tmpl);
    Nan::Set(target, Nan::New("BluetoothFd").ToLocalChecked(), Nan::GetFunction(tmpl).ToLocalChecked());
}

NODE_MODULE(BluetoothFd, BluetoothFd::Init);