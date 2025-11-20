#include "upload_sdrusbgadget.h"

#include <QFile>
#include <chrono>
#include <thread>

#ifdef WIN32
#include <Winsock2.h>
#include "libssh.h"
#else
#include <libssh/libssh.h>
#endif

#include <QDebug>

#define S_IRWXU 0x777

//-------------------------------------------------------------------------------------------
upload_sdrusbgadget::upload_sdrusbgadget()
{

}
//-------------------------------------------------------------------------------------------
bool upload_sdrusbgadget::upload(std::string ip_, std::string &err_)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    ssh_session session = ssh_new();
    if (session == nullptr) {
        err_ = "Unable to create SSH session \n";

        return false;

    }
    std::string login = "root@" + ip_;
    ssh_options_set(session, SSH_OPTIONS_HOST, login.c_str() );
    int verbosity = SSH_LOG_PROTOCOL;
    ssh_options_set(session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
    int port = 22;
    ssh_options_set(session, SSH_OPTIONS_PORT, &port);
    int rc;
    rc = ssh_connect(session);
    if (rc != SSH_OK) {
        err_ = ssh_get_error(session);

        return false;

    }
    rc = ssh_userauth_password(session, NULL, "analog");
    if (rc != SSH_AUTH_SUCCESS) {
        err_ = ssh_get_error(session);
        ssh_disconnect(session);
        ssh_free(session);

        return false;

    }

    ssh_scp scp;
    char* buffer;
    int size_file;

    scp = ssh_scp_new(session, SSH_SCP_WRITE | SSH_SCP_RECURSIVE, "/tmp");
    if (scp == NULL) {
        err_ = ssh_get_error(session);
        ssh_scp_close(scp);
        ssh_scp_free(scp);
        ssh_disconnect(session);
        ssh_free(session);

        return false;

    }
    rc = ssh_scp_init(scp);
    if (rc != SSH_OK) {
        err_ = ssh_get_error(session);
        ssh_scp_close(scp);
        ssh_scp_free(scp);
        ssh_disconnect(session);
        ssh_free(session);

        return false;

    }
    QFile file_script(":/runme.sh");
    if(file_script.isOpen()) file_script.close();
    file_script.open(QIODevice::ReadOnly);
//    if (!file_script.open(QIODevice::ReadOnly)) return false;
    size_file = file_script.size();
    rc = ssh_scp_push_file(scp, "runme.sh", size_file, S_IRWXU);
    if (rc != SSH_OK) {
        err_ = ssh_get_error(session);
        file_script.close();

        return false;

    }
    buffer = (char*)malloc(size_file);
    file_script.read(buffer, size_file);
    file_script.close();
    rc = ssh_scp_write(scp, buffer, size_file);
    if (rc != SSH_OK) {
        err_ = ssh_get_error(session);

        return false;

    }
    ssh_scp_close(scp);
    ssh_scp_free(scp);

    scp = ssh_scp_new(session, SSH_SCP_WRITE | SSH_SCP_RECURSIVE, "/etc/init.d");
    if (scp == NULL) {
        err_ = ssh_get_error(session);
        ssh_scp_close(scp);
        ssh_scp_free(scp);
        ssh_disconnect(session);
        ssh_free(session);

        return false;

    }
    rc = ssh_scp_init(scp);
    if (rc != SSH_OK) {
        err_ = ssh_get_error(session);
        ssh_scp_close(scp);
        ssh_scp_free(scp);
        ssh_disconnect(session);
        ssh_free(session);

        return false;

    }
    QFile file_start(":/S24udc");
    if(file_start.isOpen()) file_start.close();
    file_start.open(QIODevice::ReadOnly);
//    if (!file_start.open(QIODevice::ReadOnly)) return false;
    size_file = file_start.size();
    rc = ssh_scp_push_file(scp, "S24udc", size_file, S_IRWXU);
    if (rc != SSH_OK) {
        err_ = ssh_get_error(session);
        file_start.close();

        return false;

    }
    buffer = (char*)malloc(size_file);
    file_start.read(buffer, size_file);
    file_start.close();
    rc = ssh_scp_write(scp, buffer, size_file);
    if (rc != SSH_OK) {
        err_ = ssh_get_error(session);

        return false;

    }
    ssh_scp_close(scp);
    ssh_scp_free(scp);

    scp = ssh_scp_new(session, SSH_SCP_WRITE | SSH_SCP_RECURSIVE, "/usr/sbin");
    rc = ssh_scp_init(scp);
    if (rc != SSH_OK) {
        err_ = ssh_get_error(session);
        ssh_scp_close(scp);
        ssh_scp_free(scp);
        ssh_disconnect(session);
        ssh_free(session);

        return false;

    }

    QFile file_blob(":/sdr_usb_gadget");
    if(file_blob.isOpen()) file_blob.close();
    if (!file_blob.open(QIODevice::ReadOnly)) {
//        err_ = "!file_blob.open(QIODevice::ReadOnly)";
//        return false;
    }
    size_file = file_blob.size();
    rc = ssh_scp_push_file(scp, "sdr_usb_gadget", size_file, S_IRWXU);
    if (rc != SSH_OK) {
        file_blob.close();
        err_ = ssh_get_error(session);
        size_t pos = err_.find("Text file busy");
        if(pos != std::string::npos) {
            ssh_scp_close(scp);
            ssh_scp_free(scp);
            ssh_disconnect(session);
            ssh_free(session);

            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

            return true;

        }

        return false;

    }
    buffer = (char*)malloc(size_file);
    file_blob.read(buffer, size_file);
    file_blob.close();
    rc = ssh_scp_write(scp, buffer, size_file);
    if (rc != SSH_OK) {
        err_ = ssh_get_error(session);
        ssh_scp_close(scp);
        ssh_scp_free(scp);
        ssh_disconnect(session);
        ssh_free(session);

        return false;

    }
    ssh_scp_close(scp);
    ssh_scp_free(scp);

    ssh_channel channel;

    channel = ssh_channel_new(session);
    if (channel == NULL) return false;
    rc = ssh_channel_open_session(channel);
    if (rc != SSH_OK) {
        err_ = ssh_get_error(session);
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        ssh_disconnect(session);
        ssh_free(session);

        return false;

    }
    rc = ssh_channel_request_exec(channel, "/tmp/runme.sh");
    if (rc != SSH_OK) {
        err_ = ssh_get_error(session);
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        ssh_disconnect(session);
        ssh_free(session);

        return false;

    }

    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    ssh_disconnect(session);
    ssh_free(session);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    err_ = "PlutoSDR kernel patch: Ok";

    return true;
}
//-------------------------------------------------------------------------------------------
