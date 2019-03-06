//
// Created by BongsuKim on 2019-02-13.
//

#ifndef CURLPP_EASYPOOL_H
#define CURLPP_EASYPOOL_H

// std
#include <vector>


namespace curlpp
{

class EasyPool {
public:
    explicit EasyPool(std::int64_t maxsize=1000, std::int64_t buf_size=4096);
    ~EasyPool();

    int Reset(std::int64_t maxsize, std::int64_t buf_size=4096);
    curlpp::Easy* Get();

    std::int64_t current() { return current_; }
    size_t size() { return items_.size(); }

private:
    std::int64_t maxsize_;
    std::int64_t current_;

    std::vector<curlpp::Easy*> items_;

    EasyPool(const EasyPool &);
    EasyPool &operator=  (const EasyPool &);
};

}

#endif //CURLPP_EASYPOOL_H
