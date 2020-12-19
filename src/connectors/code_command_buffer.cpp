#include "code_command_buffer.h"

#include <utility>

CodeCommandBuffer::CodeCommandBuffer():
    code_list_(),
    buffer_add_pointer_(0),
    buffer_get_pointer_(0) {}

void CodeCommandBuffer::addCode(std::shared_ptr<CodeCommand> com) {
    bool pointers_differ = hasNewCode();

    code_list_[buffer_add_pointer_] = std::move(com);
    buffer_add_pointer_ = nextPos(buffer_add_pointer_);
    if (pointers_differ && !hasNewCode()) {
        buffer_get_pointer_++;
    }
}

bool CodeCommandBuffer::hasNewCode() const {
    return buffer_get_pointer_ != buffer_add_pointer_;
}

std::shared_ptr<CodeCommand> CodeCommandBuffer::getCode() {
    if (!hasNewCode())
        return nullptr;
    auto com = code_list_[buffer_get_pointer_];
    buffer_get_pointer_ = nextPos(buffer_get_pointer_);
    return com;
}

bool CodeCommandBuffer::codeIsDoubled(std::shared_ptr<CodeCommand> com) const {
    for (byte k = 0; k < CODE_BUFFER_SIZE; k++) {
        if (code_list_[k] != nullptr) {
            if (code_list_[k]->getType() == com->getType() && code_list_[k]->getCode() == com->getCode()) {
                if (code_list_[k]->getTimestamp() + CODE_TIME_GAP > com->getTimestamp()) {
                    return true;
                }
            }
        }
    }
    return false;
}

void CodeCommandBuffer::print() const {
    for (byte k = 0; k < CODE_BUFFER_SIZE; k++) {
        if (code_list_[k] != nullptr) {
            logger.print(LOG_TYPE::DATA, "");
            logger.print(k);
            logger.print(": ");
            logger.print(code_list_[k]->getTimestamp());
            logger.print(" -> ");
            logger.println(code_list_[k]->getCode());
        } else {
            logger.print(LOG_TYPE::DATA, "");
            logger.print(k);
            logger.print(": ");
            logger.println("null");
        }
    }
}