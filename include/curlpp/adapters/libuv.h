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
// TODO: 테스트 한 후 유지하기
#include <map>


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
        void *userp;                    // UVCurlData pointer
    } curlLibuvContext;


    /*
     * libuv loop 와 curl_multi_handle 주목적
     * main context
     *
     * */
    typedef struct __UVCurlData{
        uv_loop_t               *loop;
        uv_timer_t              timeout;
        curlpp::Multi           curlm;
        curlpp::Multi::Msgs     msgs;
        OnMessageDoneFunctor    cb;
        // TODO: 실제 socket 관리하는 공간
        // TODO: 애매하면 second -> curl_socket_t 로 변경하기

    } UVCurlData;


    class UVCurl {
    public:

        /*
         * initialize() 대신 constructor 에서 throw 을 던지는 것이 낫다
         * 사용자가 initialize 를 까먹고 함수마다 initialized 되었는지 확인하는 cost 가 발생
         * */
        // TODO: DI 방식으로
        explicit UVCurl(uv_loop_t* loop, size_t maxmsg=1000)
        : maxmsg_(maxmsg)
        {
            uvdata_.loop = loop;

            // msgs reserved
            uvdata_.msgs.reserve(maxmsg_);
            for (size_t idx=0; idx < maxmsg_; idx++) {
                uvdata_.msgs.emplace_back(std::make_pair(nullptr, curlpp::Multi::Info()));
            }

            // uv_timer 초기화
            if (uv_timer_init(uvdata_.loop, &(uvdata_.timeout))) {
                throw std::runtime_error("uv_timer_init error");
            }

            // 함수 할당 (class static method 로 연결) 및 함수 인자로 UVCurlData 객체를 넘김
            curl_multi_setopt(uvdata_.curlm.raw(), CURLMOPT_SOCKETFUNCTION, UVCurl::curlm_sock_cb);
            curl_multi_setopt(uvdata_.curlm.raw(), CURLMOPT_SOCKETDATA, reinterpret_cast<void*>(&uvdata_));
            curl_multi_setopt(uvdata_.curlm.raw(), CURLMOPT_TIMERFUNCTION, UVCurl::curlm_timer_cb);
            curl_multi_setopt(uvdata_.curlm.raw(), CURLMOPT_TIMERDATA, reinterpret_cast<void*>(&uvdata_));
        }

        ~UVCurl() {
        }

        void Add(const curlpp::Easy* handle) {
            uvdata_.curlm.Add(handle);
        }

        void setOnMessageDoneHandler(OnMessageDoneFunctor&& cb) {
            uvdata_.cb = std::forward<OnMessageDoneFunctor>(cb);
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
            uv_close(reinterpret_cast<uv_handle_t *>(&(context->poll_handle)), UVCurl::curlCloseCallback);
        }

        /*
         * event handler 가 호출되는 곳
         * */
        static void check_multi_info(UVCurlData* cHnd) {
            size_t nMsgs = cHnd->curlm.infos(cHnd->msgs);

            /*
             * reserved 되었고 emplace_back 을 했기 때문에 size = maxmsg_ 가 됨
             * 따라서 실제 메세지 개수는 nMsgs 로 확인해야 함
             * */
            for(size_t idx=0; idx < nMsgs; idx++ ) {
                switch (cHnd->msgs[idx].second.msg) {
                    case CURLMSG_DONE:
                        if(cHnd->cb)
                            cHnd->cb(cHnd->msgs[idx].first);

                        // callback 함수가 끝나면 해당 handler 를 지움
                        cHnd->curlm.Remove(cHnd->msgs[idx].first);
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
        static curlLibuvContext* createCurlContext(curl_socket_t sockfd, UVCurlData* cHnd) {
            // 성능 때문에 new 대신 malloc 호출
            curlLibuvContext* context = static_cast<curlLibuvContext*>(malloc(sizeof(curlLibuvContext)));
            context->sockfd = sockfd;
            context->userp = static_cast<void*>(cHnd);
            //context->userp = reinterpret_cast<UVCurlData*>(cHnd);

            uv_poll_init_socket(cHnd->loop, &context->poll_handle, sockfd);
            context->poll_handle.data = context;

            return context;
        }

        /* Called by uv_pool_t when there is an action on a socket */
        static void poll_event_cb(uv_poll_t* handle, int status, int events) {
            int running_handles;
            int flags = 0;
            curl_socket_t sockfd;

            curlLibuvContext* context = reinterpret_cast<curlLibuvContext *>(handle->data);
            UVCurlData* cHnd = reinterpret_cast<UVCurlData *>(context->userp);

            // TODO: 테스트 하고 원래 코드에 옮기고 지우기
            fprintf(stderr, "poll_event_cb: status=%d, events=%d\n", status, events);

            // TODO: 여기 앞단에 아래의 event_cb 처럼 socket already closed 가 있어야 함
            // TODO: https://github.com/curl/curl/blob/master/docs/examples/asiohiper.cpp
            //SocketIsDead
            //if (cHnd->socket_map.find(context->sockfd) == cHnd->socket_map.end()) {
            //    fprintf(stderr, "\nevent_cb: socket already closed %d", context->sockfd);
            //    return;
            //}

            // TODO: 여기는 socket(fd) 레벨인데 N:M 관계에서 어떻게 socket 이 종료 되었는지 알 수 있을까? 외부에서 끊긴 경우가 문제
            CURL* lasteasy = cHnd->curlm.EasyRawBySocket(context->sockfd);
            if (lasteasy == nullptr) {
                fprintf(stderr, "lasteasy empty\n");
                return;
            }
            CURLcode res = curl_easy_getinfo(lasteasy, CURLINFO_ACTIVESOCKET, &sockfd);
            if (res != CURLE_OK) {
                fprintf(stderr, "CURLINFO_ACTIVESOCKET error %s\n", curl_easy_strerror(res));
                return;
            }

            if (status < 0) flags = CURL_CSELECT_ERR;
            if (!status && (events & UV_READABLE)) flags |= CURL_CSELECT_IN;
            if (!status && (events & UV_WRITABLE)) flags |= CURL_CSELECT_OUT;

            curl_multi_socket_action(cHnd->curlm.raw(), context->sockfd, flags, &running_handles);

            check_multi_info(cHnd);
        }

        /*
         * CURLMOPT_SOCKETFUNCTION
         * userp: UVCurlData pointer
         * socketp: curlLibuvContext pointer
         * 처음 실행할 때는 할당되어 있지 않기 때문에 생성, 그 다음엔 상황에 따라 destroy => recreation 단계를 거침
         * */
        static int curlm_sock_cb(CURL *easy, curl_socket_t sockfd, int action, void *userp, void *socketp) {
            UVCurlData* cHnd = reinterpret_cast<UVCurlData *>(userp);
            curlLibuvContext* cCtx;
            int events = 0;

            fprintf(stderr, "\ncurlm_sock_cb: sockfd=%d, action=%d, socketp=%p\n", sockfd, action, socketp);

            switch (action) {
                case CURL_POLL_IN:
                case CURL_POLL_OUT:
                case CURL_POLL_INOUT:

                    // TODO: add sockfd
                    cHnd->curlm.AddSocket(easy, sockfd);

                    // an association in the multi handle between the given socket and a private pointer of the application
                    cCtx = socketp ? reinterpret_cast<curlLibuvContext *>(socketp) : UVCurl::createCurlContext(sockfd, cHnd);
                    curl_multi_assign(cHnd->curlm.raw(), sockfd, (void *) cCtx);

                    if (action != CURL_POLL_IN) events |= UV_WRITABLE;
                    if (action != CURL_POLL_OUT) events |= UV_READABLE;

                    uv_poll_start(&cCtx->poll_handle, events, UVCurl::poll_event_cb);

                    break;

                case CURL_POLL_REMOVE:
                    if (socketp) {
                        // need a pointer of poll_handle
                        uv_poll_stop(&(reinterpret_cast<curlLibuvContext *>(socketp)->poll_handle));
                        UVCurl::destroyCurlContext(reinterpret_cast<curlLibuvContext *>(socketp));
                        curl_multi_assign(cHnd->curlm.raw(), sockfd, nullptr);
                    }

                    break;

                default:
                    abort();
            }

            return 0;
        }

        /*
         * handle->data: UVCurlData pointer
         * */
        static void uv_timeout_cb(uv_timer_t* handle) {
            int running_handles;
            UVCurlData* cHnd = reinterpret_cast<UVCurlData *>(handle->data);
            curl_multi_socket_action(cHnd->curlm.raw(), CURL_SOCKET_TIMEOUT, 0, &running_handles);
            check_multi_info(cHnd);
        }

        /*
         * CURLMOPT_TIMERFUNCTION
         * userp: UVCurlData 의 pointer
         * */
        static int curlm_timer_cb(CURLM* multi, long timeout_ms, void *userp) {
            UVCurlData* cHnd = reinterpret_cast<UVCurlData *>(userp);

            if (timeout_ms < 0) {
                uv_timer_stop(&(cHnd->timeout));
            } else {
                if (timeout_ms == 0)
                    timeout_ms = 1; /* 0 means directly call socket_action, but we'll do it in a bit */
                cHnd->timeout.data = cHnd;
                uv_timer_start(&(cHnd->timeout), uv_timeout_cb, timeout_ms, 0);
            }
            return 0;
        }

    private:
        curlLibuvContext cCtx_;
        UVCurlData uvdata_;
        size_t maxmsg_;

        UVCurl(const UVCurl &);
        UVCurl &operator=(const UVCurl &);
    };

}

#endif //CURLPP_ADAPTERS_LIBUV_H
