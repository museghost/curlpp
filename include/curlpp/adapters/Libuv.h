//
// Created by BongsuKim on 18/09/2018.
//

#ifndef CURLPP_ADAPTERS_LIBUV_H
#define CURLPP_ADAPTERS_LIBUV_H


#include <uv.h>
#include <curl/curl.h>
#include <curlpp/Multi.hpp>
#include <curlpp/Infos.hpp>
// std
#include <functional>


namespace curlpp {

    // c++ style callback
    typedef std::function< void(const curlpp::Easy *) > OnMessageDoneFunctor;


    /*
     * curl sockfd 를 libuv 의 poll_handle 로 연결이 주목적
     *
     * */
    typedef struct __curlLibuvContext {
        uv_poll_t poll_handle;          // struct 형, poll_handle.data 에는 curlLibuvContext 자기 자신이 할당
        curl_socket_t sockfd;           // int 형
        void *userp;                    // curlLibuvHandler pointer
    } curlLibuvContext;


    /*
     * libuv loop 와 curl_multi_handle 주목적
     * main context
     *
     * */
    typedef struct __curlLibuvHandler{
        uv_loop_t               *loop;
        uv_timer_t              timeout;
        curlpp::Multi           mt;
        curlpp::Multi::Msgs     msgs;
        OnMessageDoneFunctor    cb;
    } curlLibuvHandler;


    class LibuvAdapter {
    public:

        /*
         * initialize() 대신 constructor 에서 throw 을 던지는 것이 낫다
         * 사용자가 initialize 를 까먹고 함수마다 initialized 되었는지 확인하는 cost 가 발생
         * */
        explicit LibuvAdapter(uv_loop_t* loop, size_t maxMsgs=1000) :
                maxMsgs_(maxMsgs)
        {
            cHnd_.loop = loop;

            // msgs reserved
            cHnd_.msgs.reserve(maxMsgs_);
            for (size_t idx=0; idx < maxMsgs_; idx++) {
                cHnd_.msgs.emplace_back(std::make_pair(nullptr, curlpp::Multi::Info()));
            }

            // uv_timer 초기화
            if (uv_timer_init(cHnd_.loop, &(cHnd_.timeout))) {
                throw std::runtime_error("uv_timer_init error");
            }

            // 함수 할당 (class static method 로 연결) 및 함수 인자로 curlLibuvHandler 객체를 넘김
            curl_multi_setopt(cHnd_.mt.getMHandle(), CURLMOPT_SOCKETFUNCTION, LibuvAdapter::handleSocket);
            curl_multi_setopt(cHnd_.mt.getMHandle(), CURLMOPT_SOCKETDATA, reinterpret_cast<void*>(&cHnd_));
            curl_multi_setopt(cHnd_.mt.getMHandle(), CURLMOPT_TIMERFUNCTION, LibuvAdapter::startTimeout);
            curl_multi_setopt(cHnd_.mt.getMHandle(), CURLMOPT_TIMERDATA, reinterpret_cast<void*>(&cHnd_));
        }

        ~LibuvAdapter() {
        }

        void add(const curlpp::Easy* handle) {
            cHnd_.mt.add(handle);
        }

        void setOnMessageDoneHandler(OnMessageDoneFunctor&& cb) {
            cHnd_.cb = std::forward<OnMessageDoneFunctor>(cb);
        }

        /*
         * handle->data = curlLibuvContext 의 pointer
         * */
        static void curlCloseCallback(uv_handle_t* handle) {
            // 여기는 uv_poll 만 중지 => poll_handle 하고 sockfd 만 날아감
            curlLibuvContext* context = reinterpret_cast<curlLibuvContext *>(handle->data);
            free(context);
        }

        static void destroyCurlContext(curlLibuvContext* context) {
            uv_close(reinterpret_cast<uv_handle_t *>(&(context->poll_handle)), LibuvAdapter::curlCloseCallback);
        }

        /*
         * event handler 가 호출되는 곳
         * */
        static void checkMultiInfo(curlLibuvHandler* cHnd) {
            CURLM* mHandle = cHnd->mt.getMHandle();
            size_t nMsgs = cHnd->mt.infos(cHnd->msgs);

            /*
             * reserved 되었고 emplace_back 을 했기 때문에 size = maxMsgs_ 가 됨
             * 따라서 실제 메세지 개수는 nMsgs 로 확인해야 함
             * */
            for(size_t idx=0; idx < nMsgs; idx++ ) {
                switch (cHnd->msgs[idx].second.msg) {
                    case CURLMSG_DONE:
                        if(cHnd->cb)
                            cHnd->cb(cHnd->msgs[idx].first);

                        // callback 함수가 끝나면 해당 handler 를 지움
                        cHnd->mt.remove(cHnd->msgs[idx].first);
                        break;

                    default:
                        break;

                } // end-switch

            } // end-for
        }

        /*
         * curlLibuvContext 을 생성 후 uv_poll 에 연동
         *
         * */
        static curlLibuvContext* createCurlContext(curl_socket_t sockfd, curlLibuvHandler* cHnd) {
            // 성능 때문에 new 대신 malloc 호출
            curlLibuvContext* context = static_cast<curlLibuvContext*>(malloc(sizeof(curlLibuvContext)));
            context->sockfd = sockfd;
            context->userp = static_cast<void*>(cHnd);
            //context->userp = reinterpret_cast<curlLibuvHandler*>(cHnd);

            uv_poll_init_socket(cHnd->loop, &context->poll_handle, sockfd);
            context->poll_handle.data = context;

            return context;
        }

        static void curlPerform(uv_poll_t* pollHandle, int status, int events) {
            int running_handles;
            int flags = 0;

            curlLibuvContext* context = reinterpret_cast<curlLibuvContext *>(pollHandle->data);
            curlLibuvHandler* cHnd = reinterpret_cast<curlLibuvHandler *>(context->userp);

            if (status < 0) flags = CURL_CSELECT_ERR;
            if (!status && (events & UV_READABLE)) flags |= CURL_CSELECT_IN;
            if (!status && (events & UV_WRITABLE)) flags |= CURL_CSELECT_OUT;

            curl_multi_socket_action(cHnd->mt.getMHandle(), context->sockfd, flags, &running_handles);

            checkMultiInfo(cHnd);
        }

        /*
         * curl socket 에 변화가 생기면 상황에 따라 uv 에 넘겨주는 역할
         * userp: curlLibuvHandler pointer
         * socketp: curlLibuvContext pointer (처음 실행할 때는 할당되어 있지 않기 때문에 생성, 그 다음엔 상황에 따라 destroy => recreation 단계를 거침)
         * */
        static int handleSocket(CURL *easy, curl_socket_t sockfd, int action, void *userp, void *socketp) {
            curlLibuvHandler* cHnd = reinterpret_cast<curlLibuvHandler *>(userp);
            curlLibuvContext* cCtx;
            int events = 0;

            switch (action) {
                case CURL_POLL_IN:
                case CURL_POLL_OUT:
                case CURL_POLL_INOUT:
                    cCtx = socketp ? reinterpret_cast<curlLibuvContext *>(socketp) : LibuvAdapter::createCurlContext(sockfd, cHnd);
                    curl_multi_assign(cHnd->mt.getMHandle(), sockfd, (void *) cCtx);

                    if (action != CURL_POLL_IN) events |= UV_WRITABLE;
                    if (action != CURL_POLL_OUT) events |= UV_READABLE;

                    uv_poll_start(&cCtx->poll_handle, events, LibuvAdapter::curlPerform);

                    break;

                case CURL_POLL_REMOVE:
                    if (socketp) {
                        // need a pointer of poll_handle
                        uv_poll_stop(&(reinterpret_cast<curlLibuvContext *>(socketp)->poll_handle));
                        LibuvAdapter::destroyCurlContext(reinterpret_cast<curlLibuvContext *>(socketp));
                        curl_multi_assign(cHnd->mt.getMHandle(), sockfd, nullptr);
                    }

                    break;

                default:
                    abort();
            }

            return 0;
        }

        /*
         * handle->data: curlLibuvHandler pointer
         * */
        static void onTimeout(uv_timer_t* handle) {
            int running_handles;
            curlLibuvHandler* cHnd = reinterpret_cast<curlLibuvHandler *>(handle->data);
            curl_multi_socket_action(cHnd->mt.getMHandle(), CURL_SOCKET_TIMEOUT, 0, &running_handles);
            checkMultiInfo(cHnd);
        }

        /*
         * curl_multi 에서 호출하는 callback 양식 (multi  필요)
         * userp: curlLibuvHandler 의 pointer
         * */
        static int startTimeout(CURLM* multi, long timeout_ms, void *userp) {
            curlLibuvHandler* cHnd = reinterpret_cast<curlLibuvHandler *>(userp);

            if (timeout_ms < 0) {
                uv_timer_stop(&(cHnd->timeout));
            } else {
                if (timeout_ms == 0)
                    timeout_ms = 1; /* 0 means directly call socket_action, but we'll do it in a bit */
                cHnd->timeout.data = cHnd;
                uv_timer_start(&(cHnd->timeout), onTimeout, timeout_ms, 0);
            }
            return 0;
        }

    private:
        curlLibuvContext cCtx_;
        curlLibuvHandler cHnd_;
        size_t maxMsgs_;

        LibuvAdapter(const LibuvAdapter &);
        LibuvAdapter &operator=(const LibuvAdapter &);
    };

}

#endif //CURLPP_ADAPTERS_LIBUV_H
