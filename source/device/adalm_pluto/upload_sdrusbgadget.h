#ifndef UPLOAD_SDRUSBGADGET_H
#define UPLOAD_SDRUSBGADGET_H

#include <string>

class upload_gadget_callback
{
public:
    virtual ~upload_gadget_callback(){};
    virtual void show_upload(bool set_show=true)=0;
};

class upload_sdrusbgadget
{
public:
    upload_sdrusbgadget();
    void connect_upload_gadget(upload_gadget_callback *cb_){callback_upload_gadget = cb_;};
    upload_gadget_callback *callback_upload_gadget;
    bool upload(std::string ip_, std::string &err_);
};

#endif // UPLOAD_SDRUSBGADGET_H
