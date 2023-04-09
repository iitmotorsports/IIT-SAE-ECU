#pragma once

#include "activeModule.hpp"

namespace Module {

class MessengerModule_t : public Module::ActiveModule_t {
public:
    static const int classID = 2;

    virtual int getClassID() const { return classID; }

    struct Msg_t {
    private:
        std::mutex waitMux; // IMPROVE: modules that have other modules waiting on them should be bumped up in priority
        enum Status {
            SENT,
            RECEIVED,
            REPLY,
            ERROR,
            NIL
        } status = NIL;
        Msg_t(const MessengerModule_t *sender, const MessengerModule_t *receiver, const void *value) : status(SENT), sender(sender), receiver(receiver), value(value) { waitMux.lock(); }
        Msg_t() {}

        friend MessengerModule_t;

    public:
        const MessengerModule_t *sender = 0;
        const MessengerModule_t *receiver = 0;
        const void *value;

        void wait() {
            waitMux.lock();
        }

        void timeout() {
            status = ERROR;
            waitMux.unlock();
        }

        void reply() {
            status = REPLY;
            waitMux.unlock();
        }

        void finish() {
            status = NIL;
            waitMux.unlock();
        }
    };

    using Module::ActiveModule_t::ActiveModule_t;

private:
    std::queue<Msg_t *> incoming; // TODO: allow concurrent produce and consume

    void _queueMsg(Msg_t *msg) {
        incoming.push(msg); // TODO: mutex if necessary, make modules wait to push if needed
    }

protected:
    Msg_t *sendMessage(MessengerModule_t *receiver, const void *value) {
        Msg_t *msg = new Msg_t(this, receiver, value);
        receiver->_queueMsg(msg);
        return msg;
    }

    // Wait for a reply
    void sendMessageWait(MessengerModule_t *receiver, const void *value) {
        Msg_t msg(this, receiver, value);
        receiver->_queueMsg(&msg);
        msg.wait();
    }

    // Up to the sender and receiver to send and accept the correct data/structs
    Msg_t *receiveMessage() {
        if (incoming.empty())
            return nullptr;
        Msg_t *msg = incoming.front();
        incoming.pop();
        msg->status = Msg_t::RECEIVED;
        return msg;
    }
};

} // namespace Module