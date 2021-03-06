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

#ifndef CURLPP_MULTI_HPP
#define CURLPP_MULTI_HPP


#include <curl/multi.h>

//#include <list>
#include <vector>
#include <map>


namespace curlpp {

class Easy;

class Multi {
public:
    struct Info
    {
        CURLcode code;
        CURLMSG msg;
    };

    typedef std::vector<std::pair<const curlpp::Easy*, Multi::Info> > Msgs;

    Multi();
    ~Multi();

    void Dispose();
    void Add(const curlpp::Easy* handle);
    void Remove(const curlpp::Easy* handle);
    bool perform(int * nbHandles);
    void fdset(fd_set * read_fd_set, fd_set * write_fd_set, fd_set * exc_fd_set, int * max_fd);

    Msgs ReadMsg();
    size_t ReadMsg(curlpp::Multi::Msgs& result);

    // TODO: sockfd
    void AddSocket(CURL* easy, curl_socket_t sockfd);
    CURL* EasyRawBySocket(curl_socket_t sockfd);

    const CURLM* raw() const;
    CURLM* raw();


private:
    CURLM* curlm_;
    std::map<CURL*, const curlpp::Easy*> handles_;
    // TODO: sockfd
    std::map<curl_socket_t, CURL*> sockets_;
    std::map<curl_socket_t, CURL*>::iterator sit_;
    std::map<CURL*, curl_socket_t> mapper_;
    std::map<CURL*, curl_socket_t>::iterator mit_;


    Multi(const Multi &);
    Multi &operator=(const Multi &);
};


} // namespace curlpp

namespace cURLpp = curlpp;


#endif // #ifndef CURLPP_MULTI_HPP
