#include <cstdlib>
#include <string>
#include <thread>

#include <gtest/gtest.h>

#include <nng/nng.h>
#include <nng/protocol/reqrep0/rep.h>

#include "common/commands/base_commands.pb.h"
#include "common/envelope.pb.h"
#include "kicad/kicad_connection.h"

namespace
{
    // run a minimal KiCad API REP server on the given URL, echoing GetKiCadBinaryPath requests
    void run_test_server(const std::string& url, const std::string& server_token, int* status) {
        // open a REP socket for the server
        nng_socket socket;
        if (nng_rep0_open(&socket) != 0) {
            // record the failure
            *status = 1;
            // exit
            return;
        }
        // listen on the provided URL
        if (nng_listen(socket, url.c_str(), nullptr, 0) != 0) {
            // record the failure
            *status = 2;
            // close the socket
            nng_close(socket);
            // exit
            return;
        }
        // receive the request
        void* data = nullptr;
        size_t size = 0;
        if (nng_recv(socket, &data, &size, NNG_FLAG_ALLOC) != 0) {
            // record the failure
            *status = 3;
            // close the socket
            nng_close(socket);
            // exit
            return;
        }
        // parse the request envelope
        kiapi::common::ApiRequest request;
        const bool parsed = request.ParseFromArray(data, static_cast<int>(size));
        // free the received buffer
        nng_free(data, size);
        // record the failure when parsing failed
        if (!parsed) {
            *status = 4;
            // close the socket
            nng_close(socket);
            // exit
            return;
        }
        // unpack the command payload
        kiapi::common::commands::GetKiCadBinaryPath command;
        const bool unpacked = request.message().UnpackTo(&command);
        // record the failure when the payload was not the expected command
        if (!unpacked) {
            *status = 5;
            // close the socket
            nng_close(socket);
            // exit
            return;
        }
        // build the response envelope
        kiapi::common::ApiResponse response;
        // echo the server token
        response.mutable_header()->set_kicad_token(server_token);
        // report success
        response.mutable_status()->set_status(kiapi::common::AS_OK);
        // fill the path response with the requested binary name
        kiapi::common::commands::PathResponse path_response;
        path_response.set_path("/usr/local/bin/" + command.binary_name());
        // pack the path response into the envelope
        response.mutable_message()->PackFrom(path_response);
        // serialize the response
        std::string payload;
        if (!response.SerializeToString(&payload)) {
            // record the failure
            *status = 6;
            // close the socket
            nng_close(socket);
            // exit
            return;
        }
        // send the serialized response
        if (nng_send(socket, payload.data(), payload.size(), 0) != 0) {
            // record the failure
            *status = 7;
            // close the socket
            nng_close(socket);
            // exit
            return;
        }
        // record success
        *status = 0;
        // close the socket
        nng_close(socket);
    }

    // unique ipc:// url for a single test
    std::string make_test_url(int seed) {
        // return a per-seed unix domain socket url
        return "ipc:///tmp/kicad_xyce_test_" + std::to_string(seed) + ".sock";
    }

#ifdef _WIN32
    // _putenv_s is the CRT replacement for setenv; passing an empty value removes the variable
    void set_environment_variable(const char* name, const char* value, int) {
        _putenv_s(name, value);
    }
    void unset_environment_variable(const char* name) {
        _putenv_s(name, "");
    }
#else
    void set_environment_variable(const char* name, const char* value, int overwrite) {
        ::setenv(name, value, overwrite);
    }
    void unset_environment_variable(const char* name) {
        ::unsetenv(name);
    }
#endif
} // namespace

TEST(KiCadConnectionChecks, is_kicad_mode_false_when_environment_unset) {
    // clear the environment variables
    unset_environment_variable("KICAD_API_SOCKET");
    unset_environment_variable("KICAD_API_TOKEN");
    // assert
    ASSERT_FALSE(KiCadConnection::is_kicad_mode());
}

TEST(KiCadConnectionChecks, is_kicad_mode_true_when_environment_set) {
    // set both environment variables
    set_environment_variable("KICAD_API_SOCKET", "ipc:///tmp/example.sock", 1);
    set_environment_variable("KICAD_API_TOKEN", "example-token", 1);
    // assert
    ASSERT_TRUE(KiCadConnection::is_kicad_mode());
    // clean up the environment
    unset_environment_variable("KICAD_API_SOCKET");
    unset_environment_variable("KICAD_API_TOKEN");
}

TEST(KiCadConnectionChecks, is_kicad_mode_false_when_socket_missing) {
    // set only the token
    unset_environment_variable("KICAD_API_SOCKET");
    set_environment_variable("KICAD_API_TOKEN", "example-token", 1);
    // assert
    ASSERT_FALSE(KiCadConnection::is_kicad_mode());
    // clean up the environment
    unset_environment_variable("KICAD_API_TOKEN");
}

TEST(KiCadConnectionChecks, from_environment_returns_null_when_standalone) {
    // clear the environment variables
    unset_environment_variable("KICAD_API_SOCKET");
    unset_environment_variable("KICAD_API_TOKEN");
    // act
    const auto connection = KiCadConnection::from_environment();
    // assert
    ASSERT_EQ(connection, nullptr);
}

TEST(KiCadConnectionChecks, exchanges_binary_path_request_with_server) {
    // url for the test server
    const std::string url = make_test_url(1);
    // server status code
    int server_status = -1;
    // run the server on a background thread
    std::thread server(run_test_server, url, "server-token", &server_status);
    // give the server a moment to start listening
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // create a connection to the test server
    KiCadConnection connection(url, "client-token", "test-client");
    // resolve the kicad-cli path through the server
    const std::string path = connection.get_kicad_binary_path("kicad-cli");
    // wait for the server to finish
    server.join();
    // assert
    ASSERT_EQ(server_status, 0);
    ASSERT_EQ(path, "/usr/local/bin/kicad-cli");
}
