#pragma once
#include <cstdlib>
#include <map>
#include <mutex>
#include <string>
#include <vector>


template <typename Key, typename Value>
class ConcurrentMap {
private:
    struct Bucket {
        std::mutex mutex;
        std::map<Key, Value> map;
    };

public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    
    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;

        Access(const Key& key, Bucket& bucket);
    };

    explicit ConcurrentMap(size_t bucket_count);


    Access operator[](const Key& key) {
        auto& bucket = buckets_[static_cast<uint64_t>(key) % buckets_.size()];
        return { key, bucket };
    }

    std::size_t erase(const Key& key);

    std::map<Key, Value> BuildOrdinaryMap();

private:
    std::vector<Bucket> buckets_;
};



template <typename Key, typename Value>
ConcurrentMap<Key, Value>::Access::Access(const Key& key, ConcurrentMap<Key, Value>::Bucket& bucket)
    : guard(bucket.mutex)
    , ref_to_value(bucket.map[key]) {
}

template <typename Key, typename Value>
ConcurrentMap<Key, Value>::ConcurrentMap(size_t bucket_count)
    : buckets_(bucket_count) {
}



template <typename Key, typename Value>
std::size_t ConcurrentMap<Key, Value>::erase(const Key& key) {
    auto bucket = buckets_[std::abs(static_cast<int>(key % buckets_.size()))]; 
    std::lock_guard lock_guard_bucket(bucket.mutex_);
    return bucket.map_.erase(key);
}

template <typename Key, typename Value>
std::map<Key, Value> ConcurrentMap<Key, Value>::BuildOrdinaryMap() {
    std::map<Key, Value> result;
    for (auto& [mutex, map] : buckets_) {
        std::lock_guard g(mutex);
        result.insert(map.begin(), map.end());
    }
    return result;
}


