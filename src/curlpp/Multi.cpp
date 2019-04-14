/*
 *    Copyright (c) <2002-2009> <Jean-Philippe Barrette-LaPierre>
 *    
 *    Permission is hereby granted, free of charge, to any person obtaining
 *    a copy of this software and associated documentation files 
 *    (curlpp), to deal in the Software without restriction, 
 *    including without limitation the rights to use, copy, modify, merge,
 *    publish, distribute, sublicense, and/or sell copies of the Software,
 *    and to permit persons to whom the Software is furnished to do so, 
 *    subject to the following conditions:
 *    
 *    The above copyright notice and this permission notice shall be included
 *    in all copies or substantial portions of the Software.
 *    
 *    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 *    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
 *    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
 *    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "curlpp/Easy.hpp"
#include "curlpp/Exception.hpp"
#include "curlpp/Multi.hpp"

namespace curlpp {
    Multi::Multi() {
        curlm_ = curl_multi_init();
        runtimeAssert("Error when trying to curl_multi_init() a handle", curlm_ != nullptr);
    }

    Multi::~Multi() {
        // remove all the remaining easy handles
        //while (!handles_.empty())
        for (const auto& it: handles_) {

            //std::map<CURL*, const Easy *>::iterator handle = handles_.begin();
            // if caller deletes request before, this would raise EXC_BAD_ACCESS error using handle->second->getHandle()
            //curl_multi_remove_handle(curlm_, handle->first);
            curl_multi_remove_handle(curlm_, it.first);
            //handles_.erase(handle);
        }

        handles_.clear();
        curl_multi_cleanup(curlm_);
    }

    void Multi::Dispose() {

        for (const auto& it: handles_) {
            curl_multi_remove_handle(curlm_, it.first);
        }

        handles_.clear();
        curl_multi_cleanup(curlm_);
    }

    const CURLM* Multi::raw() const {
        return curlm_;
    }

    CURLM* Multi::raw() {
        return curlm_;
    }

    void Multi::Add(const Easy * handle) {
        CURLMcode code = curl_multi_add_handle(curlm_, handle->getHandle());
        if(code != CURLM_CALL_MULTI_PERFORM) {
            if(code != CURLM_OK) {
                throw RuntimeError(curl_multi_strerror(code));
            }
        }
        handles_.insert(std::make_pair(handle->getHandle(), handle));
        // TODO: 여기서 sockfd 를 가져올 수 있을까?
    }

    void Multi::Remove(const Easy * handle){
        CURLMcode code = curl_multi_remove_handle(curlm_, handle->getHandle());
        if(code != CURLM_CALL_MULTI_PERFORM) {
            if(code != CURLM_OK) {
                throw RuntimeError(curl_multi_strerror(code));
            }
        }
        handles_.erase(handle->getHandle());
    }

    bool
    Multi::perform(int * nbHandles)
    {
        CURLMcode code = curl_multi_perform(curlm_, nbHandles);
        if(code == CURLM_CALL_MULTI_PERFORM) {
            return false;
        }

        if(code != CURLM_OK) {
            throw RuntimeError(curl_multi_strerror(code));
        }

        return true;
    }

    void
    Multi::fdset(fd_set * read, fd_set * write, fd_set * exc, int * max)
    {
        CURLMcode code = curl_multi_fdset(curlm_, read, write, exc, max);
        if(code != CURLM_CALL_MULTI_PERFORM) {
            if(code != CURLM_OK) {
                throw RuntimeError(curl_multi_strerror(code));
            }
        }
    }

    Multi::Msgs
    Multi::ReadMsg() {
        CURLMsg * msg; /* for picking up messages with the transfer status */

        int msgsInQueue;
        Msgs result;
        while ((msg = curl_multi_info_read(curlm_, &msgsInQueue)) != NULL) {
            Multi::Info inf;
            inf.msg = msg->msg;
            inf.code = msg->data.result;
            result.emplace_back(std::make_pair(handles_[msg->easy_handle],inf));
        }

        return result;
    }

    size_t Multi::ReadMsg(Multi::Msgs& result) {
        size_t idx = 0;
        int pending;
        // CURL* easy_handle;
        CURLMsg* message; /* for picking up messages with the transfer status */

        do {
            if((message = curl_multi_info_read(curlm_, &pending))) {
                switch(message->msg) {
                    case CURLMSG_DONE:
                        // easy_handle = message->easy_handle;
                        result[idx].first = handles_.at(message->easy_handle);
                        result[idx].second.msg = message->msg;
                        result[idx].second.code = message->data.result;
                        idx++;
                        break;
                    default:
                        break;
                }

            } // ene-if
        } while(pending);

#if 0

        while ((msg = curl_multi_info_read(curlm_, &msgsInQueue)) != NULL) {

            result[idx].first = handles_[msg->easy_handle];
            result[idx].second.msg = msg->msg;
            result[idx].second.code = msg->data.result;
            idx++;
        }
#endif

        return idx;
    }

    void Multi::AddSocket(CURL* easy, curl_socket_t sockfd) {
        // It is possible that each easy handler could share a same socket
        // So, the key should be easy handler

        // 여기는 sockfd -> last easy handler
        sit_ = sockets_.find(sockfd);
        if (sit_ != sockets_.end()) {
            // found
            sit_->second = easy;
        } else {
            // new
            sockets_.insert(std::pair<curl_socket_t, CURL*>(sockfd, easy));
        }

        // 여기는 easy handler -> several sockfds ...
        mit_ = mapper_.find(easy);
        if (mit_ != mapper_.end()) {
            // found
            mit_->second = sockfd;
        } else {
            // new
            mapper_.insert(std::pair<CURL*, curl_socket_t>(easy, sockfd));
        }
    }

    CURL* Multi::EasyRawBySocket(curl_socket_t sockfd) {
        sit_ = sockets_.find(sockfd);
        if (sit_ == sockets_.end()) {
            return nullptr;
        } else {
            return sit_->second;
        }
    }
}
