#ifndef THREADSAFE_LOOKUP_TABLE_HPP
#define THREADSAFE_LOOKUP_TABLE_HPP
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>

template <typename Key, typename Value, typename Hash = std::hash<Key>>
class threadsafe_lookup_table {
private:
    class bucket_type {
    public:
        using bucket_value = std::pair<Key, Value>;
        using bucket_data = std::list<bucket_value>;
        using bucket_iterator = typename bucket_data::iterator;

        Value value_for(const Key& key, const Value& default_value) const {
            // 获取共享锁，因为只需要读
            std::shared_lock lock(mutex_);
            const bucket_iterator found_entry = find_entry_for(key);
            return found_entry == data_.end() ? default_value : found_entry->second;
        }

        void add_or_update_mapping(const Key& key, const Value& value) {
            // 获取独占锁，因为需要写
            std::unique_lock lock(mutex_);
            const bucket_iterator found_entry = find_entry_for(key);
            if (found_entry == data_.end()) {
                data_.push_back(bucket_value(key, value));
            } else {
                found_entry->second = value;
            }
        }

        void remove_mapping(const Key& key) {
            // 获取独占锁，因为需要写
            std::unique_lock lock(mutex_);
            const bucket_iterator found_entry = find_entry_for(key);
            if (found_entry != data_.end()) {
                data_.erase(found_entry);
            }
        }

    private:
        bucket_iterator find_entry_for(const Key& key) {
            // 不加锁
            return std::find_if(data_.begin(), data_.end(), [&](const bucket_value& item) {
                return item.first == key;
            });
        }

    private:
        bucket_data data_;

        //
        mutable std::shared_mutex mutex_;
    };

public:
    using key_type = Key;
    using mapped_type = Value;
    using hash_type = Hash;

    // 由于在构造函数中初始化了 buckets_，并且这个 vector 不会变，所以不需要加锁

    explicit threadsafe_lookup_table(unsigned num_buckets = 19, const Hash& hasher = Hash()) :
        buckets_(num_buckets), hasher_(hasher) {
        for (unsigned i = 0; i < num_buckets; ++i) {
            buckets_[i].reset(new bucket_type);
        }
    }

    threadsafe_lookup_table(const threadsafe_lookup_table&) = delete;
    threadsafe_lookup_table& operator=(const threadsafe_lookup_table&) = delete;

    Value value_for(const Key& key, const Value& default_value) const {
        return get_bucket(key).value_for(key, default_value);
    }

    void add_or_update_mapping(const Key& key, const Value& value) {
        get_bucket(key).add_or_update_mapping(key, value);
    }

    void remove_mapping(const Key& key) {
        get_bucket(key).remove_mapping(key);
    }

    std::map<Key, Value> get_map() const {
        // 对所有的互斥加锁
        // 每次按照相同的顺序（从小到大）加锁，可以避免死锁

        // 这里加的是独占锁，应该可以改成共享锁
        std::vector<std::unique_lock<std::shared_mutex>> locks;
        for (auto& bucket : buckets_) {
            locks.push_back(std::unique_lock<std::shared_mutex>(bucket->mutex_));
        }

        std::map<Key, Value> res;
        for (auto& bucket : buckets_) {
            for (const auto& item : bucket->data_) {
                res.insert(item);
            }
        }
        return res;
    }

private:
    bucket_type& get_bucket(const Key& key) {
        const std::size_t bucket_index = hasher_(key) % buckets_.size();
        return *buckets_[bucket_index];
    }

private:
    std::vector<std::unique_ptr<bucket_type>> buckets_;
    Hash hasher_;
};

#endif // THREADSAFE_LOOKUP_TABLE_HPP
