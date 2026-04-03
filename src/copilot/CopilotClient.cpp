#include "CopilotClient.h"

CopilotClient::CopilotClient(std::shared_ptr<grpc::Channel> channel)
    : stub_(copilot::AnimationCopilot::NewStub(channel)) {}

std::string CopilotClient::ExecutePrompt(const std::string& prompt) {
    copilot::PromptRequest request;
    request.set_prompt(prompt);

    copilot::PromptResponse reply;
    grpc::ClientContext context;

    grpc::Status status = stub_->ExecutePrompt(&context, request, &reply);

    if (status.ok()) {
        return reply.reply_text();
        // Here we will eventually also parse reply.actions() and apply them in Friction
    } else {
        return "RPC failed: " + status.error_message();
    }
}
