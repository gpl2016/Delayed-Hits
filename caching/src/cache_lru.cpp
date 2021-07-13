// STD headers
#include <assert.h>
#include <list>
#include <string>
#include <unordered_map>
#include <vector>

// Custom headers
#include "cache_base.hpp"
#include "cache_common.hpp"
#include "utils.hpp"

using namespace caching;

/**
 * Represents a single set (row) in an LRU-based cache.
 */
class LRUCacheSet : public BaseCacheSet {
protected:
    LRUQueue<CacheEntry> queue_; // LRU Queue

public:
    LRUCacheSet(const size_t num_entries) : BaseCacheSet(num_entries) {}
    virtual ~LRUCacheSet() {}


    virtual CacheEntry
    writehalf(const std::string& key, const utils::Packet& packet) override {
        SUPPRESS_UNUSED_WARNING(packet);
        CacheEntry written_entry;
        CacheEntry evicted_entry;
        std::cout<<"lru _  write half"<<std::endl;
        // If a corresponding entry exists, update it
        auto position_iter = queue_.positions().find(key);//queue_是LRU队列，queue_.positions()是unordered_map，返回值：如果给定的键存在于unordered_map中，则它向该元素返回一个迭代器，否则返回映射迭代器的末尾。
        //std::unordered_map<std::string, Iterator>& positions()
        if (position_iter != queue_.positions().end()) {
            std::cout<<"If a corresponding entry in or fetching , update it"<<std::endl;
            written_entry = *(position_iter->second);//position_iter是迭代器

            // Sanity checks健全性检查
            //assert(contains(key));
            assert(written_entry.isValid());
            assert(written_entry.key() == key);
            // If the read was successful, the corresponding entry is
            // the MRU element in the cache. Remove it from the queue.
            queue_.erase(position_iter);
        }
            // The update was unsuccessful, create a new entry to insert
        else {
            std::cout<<"The update was unsuccessful, create a new entry to insert _key:"<<key<<std::endl;
            written_entry.update(key);
            written_entry.toggleValid();

            // If required, evict the LRU entry
            //如果需要，逐出LRU入口     重要
//            std::cout<<"queue_.size():"<<queue_.size()<<std::endl;
//            std::cout<<"getNumEntries()"<<getNumEntries()<<std::endl;
            if (queue_.size() > getNumEntries()-1) {
                evicted_entry = queue_.popFront();
                assert(evicted_entry.isValid()); // Sanity check
                std::cout<<"half evicted_entry.key():"<<evicted_entry.key()<<"evicted_entry.status:"<<id_status_.find(evicted_entry.key())->second<<std::endl;
                if(id_status_.find(evicted_entry.key())->second=="in"){
                    setIntoOut(evicted_entry.key());
                    occupied_entries_set_.erase(evicted_entry.key());
                }
                //Todo 如果是fetching怎么办，正在fetching的被替换出去

            }

            // Update the occupied entries set
            //occupied_entries_set_.insert(key);
            std::cout<<"half Update the occupied entries set _key:"<<key<<std::endl;
        }


        // Finally, (re-)insert the written entry at the back
        queue_.insertBack(written_entry);
        // Todo fetching 的，在half中，如果被重复访问，也是先把链表前面的删了，再在后面加上。
        std::cout<<"half queue_.size():"<<queue_.size()<<std::endl;
        std::cout<<"half getNumEntries()"<<getNumEntries()<<std::endl;
        //想输出cache中的内容
//        std::list<CacheEntry>::iterator p1;
//
//        for(p1=queue_.entries().begin();p1!=queue_.entries().end();p1++){
//            std::cout<<(*p1).key() <<" "<<std::endl;
//        }
        std::unordered_set<std::string > :: iterator  p2;
        for(p2=occupied_entries_set_.begin();p2!=occupied_entries_set_.end();p2++){
            std::cout<<"half occupied_entries_set_ output:"<<*(p2)<<std::endl;
        }


        // std::cout<<queue_.entries();
        // Sanity checks
        assert(occupied_entries_set_.size() <= getNumEntries());
        assert(occupied_entries_set_.size() <= queue_.size());
        return written_entry;
    }





    /**
     * Simulates a cache write.
     *
     * @param key The key corresponding to this write request.
     * @param packet The packet corresponding to this write request.
     * @return The written CacheEntry instance.
     */
    virtual CacheEntry
    write(const std::string& key, const utils::Packet& packet) override {
        SUPPRESS_UNUSED_WARNING(packet);
        CacheEntry written_entry;
        CacheEntry evicted_entry;
        std::cout<<"lru _ write"<<std::endl;
        // If a corresponding entry exists, update it
        auto position_iter = queue_.positions().find(key);//queue_是LRU队列，queue_.positions()是unordered_map，返回值：如果给定的键存在于unordered_map中，则它向该元素返回一个迭代器，否则返回映射迭代器的末尾。
        //std::unordered_map<std::string, Iterator>& positions()
        //std::list<T>::iterator Iterator;
        //
        if(id_status_.find(key)->second=="in"&&position_iter != queue_.positions().end()){
            std::cout<<"If a corresponding entry exists, update it"<<std::endl;
            written_entry = *(position_iter->second);//position_iter是迭代器
            assert(contains(key));
            assert(written_entry.isValid());
            assert(written_entry.key() == key);
            queue_.erase(position_iter);
            queue_.insertBack(written_entry);
        }
        else if(id_status_.find(key)->second=="fetching"){
            //还需要看一下缓存大小够不够
            if (queue_.size() > getNumEntries()-1) {
                evicted_entry = queue_.popFront();
                assert(evicted_entry.isValid()); // Sanity check
                std::cout<<"evicted_entry.key():"<<evicted_entry.key()<<"evicted_entry.status:"<<id_status_.find(evicted_entry.key())->second<<std::endl;
                if(id_status_.find(evicted_entry.key())->second=="in"){
                    setIntoOut(evicted_entry.key());
                    occupied_entries_set_.erase(evicted_entry.key());
                }

                //Todo 如果是fetching怎么办，正在fetching的被替换出去  这就是问题所在了

            }
            setFetchingtoIn(key);
            occupied_entries_set_.insert(key);
        }
        else if(id_status_.find(key)->second=="out"){//这种情况比较少，z=1或z=0吧
            if (queue_.size() > getNumEntries()-1) {
                evicted_entry = queue_.popFront();
                assert(evicted_entry.isValid()); // Sanity check
                std::cout<<"stat=out  evicted_entry.key():"<<evicted_entry.key()<<"stat=out  evicted_entry.status:"<<id_status_.find(evicted_entry.key())->second<<std::endl;
                if(id_status_.find(evicted_entry.key())->second=="in"){
                    setIntoOut(evicted_entry.key());
                    occupied_entries_set_.erase(evicted_entry.key());
                }

                //Todo 如果是fetching怎么办，正在fetching的被替换出去

            }
            queue_.insertBack(written_entry);//为了更新LRU链表
            setOuttoIn(key);
            occupied_entries_set_.insert(key);
        }
        std::cout<<"queue_.size():"<<queue_.size()<<std::endl;
        std::cout<<"getNumEntries()"<<getNumEntries()<<std::endl;
        //想输出cache中的内容


        std::unordered_set<std::string > :: iterator  p2;
        for(p2=occupied_entries_set_.begin();p2!=occupied_entries_set_.end();p2++){
            std::cout<<"occupied_entries_set_ output:"<<*(p2)<<std::endl;
        }
        // std::cout<<queue_.entries();
        // Sanity checks
        assert(occupied_entries_set_.size() <= getNumEntries());
        assert(occupied_entries_set_.size() <= queue_.size());
        return written_entry;
    }
//-----------------------------------------------------------------------------
//        if (position_iter != queue_.positions().end()) {
//            std::cout<<"If a corresponding entry exists, update it"<<std::endl;
//            written_entry = *(position_iter->second);//position_iter是迭代器
//            assert(id_status_.find(key)->second=="in");
//            // Sanity checks健全性检查
//            assert(contains(key));
//            assert(written_entry.isValid());
//            assert(written_entry.key() == key);
//            // If the read was successful, the corresponding entry is
//            // the MRU element in the cache. Remove it from the queue.
//            queue_.erase(position_iter);
//
//        }
//        // The update was unsuccessful, create a new entry to insert
//        else {
//            std::cout<<"The update was unsuccessful, create a new entry to insert _key:"<<key<<std::endl;
//            written_entry.update(key);
//            written_entry.toggleValid();
//
//            // If required, evict the LRU entry
//            //如果需要，逐出LRU入口     重要
////            std::cout<<"queue_.size():"<<queue_.size()<<std::endl;
////            std::cout<<"getNumEntries()"<<getNumEntries()<<std::endl;
//            if (queue_.size() > getNumEntries()-1) {
//                evicted_entry = queue_.popFront();
//                assert(evicted_entry.isValid()); // Sanity check
//                assert(id_status_.find(key)->second=="in");
//                std::cout<<"evicted_entry.key():"<<evicted_entry.key()<<std::endl;
//                setIntoOut(evicted_entry.key());
//                occupied_entries_set_.erase(evicted_entry.key());
//            }
//
//            // Update the occupied entries set
//            id_status_.erase(key);
//            id_status_.insert({key,"in"});
//            occupied_entries_set_.insert(key);
//            std::cout<<"Update the occupied entries set _key:"<<key<<std::endl;
//        }
//
//
//        // Finally, (re-)insert the written entry at the back
//        queue_.insertBack(written_entry);
//        std::cout<<"queue_.size():"<<queue_.size()<<std::endl;
//        std::cout<<"getNumEntries()"<<getNumEntries()<<std::endl;
//        //想输出cache中的内容
//
//
//        std::unordered_set<std::string > :: iterator  p2;
//        for(p2=occupied_entries_set_.begin();p2!=occupied_entries_set_.end();p2++){
//            std::cout<<"occupied_entries_set_ output:"<<*(p2)<<std::endl;
//        }
//       // std::cout<<queue_.entries();
//        // Sanity checks
//        assert(occupied_entries_set_.size() <= getNumEntries());
//        assert(occupied_entries_set_.size() <= queue_.size());
//        return written_entry;
//    }

    /**
     * Simulates a sequence of cache writes for a particular flow's packet queue.
     * Invoking this method should be functionally equivalent to invoking write()
     * on every queued packet; this simply presents an optimization opportunity
     * for policies which do not distinguish between single/multiple writes.
     *
     * @param queue The queued write requests.
     * @return The written CacheEntry instance.
     */
    virtual CacheEntry
    writeq(const std::list<utils::Packet>& queue) override {
        const utils::Packet& packet = queue.back();
        return write(packet.getFlowId(), packet);
    }
};

/**
 * Implements a single-tiered LRU cache.
 */
class LRUCache : public BaseCache {
public:
    LRUCache(const size_t miss_latency, const size_t cache_set_associativity, const size_t
    num_cache_sets, const bool penalize_insertions, const HashType hash_type, int
             argc, char** argv) : BaseCache(miss_latency, cache_set_associativity,
                                            num_cache_sets, penalize_insertions, hash_type) {
        SUPPRESS_UNUSED_WARNING(argc);
        SUPPRESS_UNUSED_WARNING(argv);

        // Initialize the cache sets
        for (size_t idx = 0; idx < kMaxNumCacheSets; idx++) {
            cache_sets_.push_back(new LRUCacheSet(kCacheSetAssociativity));
        }
    }
    virtual ~LRUCache() {}

    /**
     * Returns the canonical cache name.
     */
    virtual std::string name() const override { return "LRUCache"; }
};

// Run default benchmarks
int main(int argc, char** argv) {
    BaseCache::defaultBenchmark<LRUCache>(argc, argv);
}
