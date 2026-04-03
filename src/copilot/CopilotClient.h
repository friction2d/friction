#ifndef COPILOT_CLIENT_H
#define COPILOT_CLIENT_H

#include <grpcpp/grpcpp.h>
#include "copilot.grpc.pb.h"
#include <string>

class CopilotClient {
public:
    CopilotClient(std::shared_ptr<grpc::Channel> channel);

    // Call the server with a prompt
    std::string ExecutePrompt(const std::string& prompt);

private:
    std::unique_ptr<copilot::AnimationCopilot::Stub> stub_;
};

#endif // COPILOT_CLIENT_H
