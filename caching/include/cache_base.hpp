#ifndef cache_base_h
#define cache_base_h

// STD headers
#include <assert.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>

// Boost headers
#include <boost/bimap.hpp>
#include <boost/program_options.hpp>

// Custom headers
#include "utils.hpp"
#include "cache_common.hpp"

namespace caching {

/**
 * Abstract base class representing a generic cache-set.
 */
class BaseCacheSet {
protected:
    const size_t kNumEntries; // The number of cache entries in this set
    std::unordered_set<std::string> occupied_entries_set_; // Set of currently cached flow IDs.当前序号缓存的flow id集合，也就是已经缓存的
public:
    BaseCacheSet(const size_t num_entries) : kNumEntries(num_entries) {std::cout<<"cache size"<<num_entries<<"  "<<kNumEntries<<std::endl;}//每个缓存里面的大小  但是一直没找到什么时候赋值的啊  ？？？
    //最后的赋值是cache size
    virtual ~BaseCacheSet() {}

    // Membership test (internal use only)
    bool contains(const std::string& flow_id) const {
        return (occupied_entries_set_.find(flow_id) !=
                occupied_entries_set_.end());
    }
    /**
     * Returns the number of cache entries in this set
     */
    size_t getNumEntries() const { return kNumEntries; }

    /**
     * Records arrival of a new packet.
     */
    virtual void recordPacketArrival(const utils::Packet& packet) {
        SUPPRESS_UNUSED_WARNING(packet);
    }

    /**
     * Simulates a cache write.
     *
     * @param key The key corresponding to this write request.
     * @param packet The packet corresponding to this write request.
     * @return The written CacheEntry instance.写入的CacheEntry实例。
     */
    virtual CacheEntry
    write(const std::string& key, const utils::Packet& packet) = 0;

    virtual CacheEntry
    writehalf(const std::string& key, const utils::Packet& packet) = 0;
    /**
     * Simulates a sequence of cache writes for a particular flow's packet queue.
     * Invoking this method should be functionally equivalent to invoking write()
     * on every queued packet; this simply presents an optimization opportunity
     * for policies which do not distinguish between single/multiple writes.
     *模拟特定流的数据包队列的缓存写入序列。
调用此方法在功能上应等同于调用write（）
在每个排队包上；这只是为不区分单写/多写的策略提供了一个优化机会。
     * @param queue The queued write requests.
     * @return The written CacheEntry instance.
     */
    virtual CacheEntry
    writeq(const std::list<utils::Packet>& queue) {
        CacheEntry written_entry;
        for (const auto& packet : queue) {
            written_entry = write(packet.getFlowId(), packet);
        }
        return written_entry;
    }
};

/**
 * Abstract base class representing a single-tiered cache.
 */
class BaseCache {
protected:
    const size_t kCacheMissLatency;         // Cost (in cycles) of an L1 cache miss
    const size_t kMaxNumCacheSets;          // Maximum number of sets in the L1 cache
    const size_t kMaxNumCacheEntries;       // Maximum number of entries in the L1 cache
    const size_t kCacheSetAssociativity;    // Set-associativity of the L1 cache
    const size_t kIsPenalizeInsertions;     // Whether insertions should incur an L1 cache miss插入是否应导致一级缓存未命中
    const HashFamily kHashFamily;           // A HashFamily instance
    //const size_t num_cache_entries;
    size_t clk_ = 0; // Time in clock cycles
    size_t total_latency_ = 0; // Total packet latency
    std::vector<BaseCacheSet*> cache_sets_; // Fixed-sized array of CacheSet instances 固定大小的缓存集实例数组
    std::unordered_set<std::string> memory_entries_; // Set of keys in the global store
    boost::bimap<size_t, std::string> completed_reads_; // A dictionary mapping clk values to the keys whose blocking reads complete on that cycle.
    //将clk值映射到其阻塞读取在该循环中完成的键的字典。
    std::unordered_map<std::string, std::list<utils::Packet>>
    packet_queues_; // Dictionary mapping keys to queued requests. Each queue
                    // contains zero or more packets waiting to be processed.
                    ////将键映射到排队请求的字典。每个队列
    ////包含零个或多个等待处理的数据包。
public:
    BaseCache(const size_t miss_latency, const size_t cache_set_associativity, const size_t
    num_cache_sets, const bool penalize_insertions, const HashType hash_type):
            kCacheMissLatency(miss_latency), kMaxNumCacheSets(num_cache_sets),
            kMaxNumCacheEntries(cache_set_associativity),
            kCacheSetAssociativity(cache_set_associativity),
            kIsPenalizeInsertions(penalize_insertions),
            kHashFamily(1, hash_type) {}

    virtual ~BaseCache() {
        // Deallocate the cache sets
        assert(cache_sets_.size() == kMaxNumCacheSets);
        for (size_t idx = 0; idx < kMaxNumCacheSets; idx++) { delete(cache_sets_[idx]); }
    }

    /**
     * Returns the canonical cache name.返回规范缓存名称。
     */
    virtual std::string name() const = 0;

    /**
     * Records arrival of a new packet.记录新数据包的到达。
     */
    virtual void recordPacketArrival(const utils::Packet& packet) {
        SUPPRESS_UNUSED_WARNING(packet);
    }

    /**
     * Returns the cache miss latency.
     */
    size_t getCacheMissLatency() const { return kCacheMissLatency; }

    /**
     * Returns the number of entries in the memory at any instant.
     * 返回内存中任何时刻的条目数。
     */
    size_t getNumMemoryEntries() const { return memory_entries_.size(); }

    /**
     * Returns the current time in clock cycles.
     */
    size_t clk() const { return clk_; }


    /**
     * Returns the total packet latency for this simulation.
     */
    size_t getTotalLatency() const { return total_latency_; }

    /**
     * Returns the cache index corresponding to the given key.
     */
    size_t getCacheIndex(const std::string& key) const {
        return (kMaxNumCacheSets == 1) ?
            0 : kHashFamily.hash(0, key) % kMaxNumCacheSets;
    }

    /**
     * Increments the internal clock.
     */
    void incrementClk() { clk_++; }

    /**
     * Handles any blocking read completions on this cycle.
     * 处理此循环中的所有阻塞读取完成。
     */
    void processAll(std::list<utils::Packet>& processed_packets) {

        /*A blocking read completed on this cycle在此循环中完成的阻塞读取
         *https://blog.csdn.net/weixin_30855099/article/details/96899976?utm_medium=distribute.pc_relevant.none-task-blog-2%7Edefault%7EBlogCommendFromMachineLearnPai2%7Edefault-5.control&depth_1-utm_source=distribute.pc_relevant.none-task-blog-2%7Edefault%7EBlogCommendFromMachineLearnPai2%7Edefault-5.control
         * completed_reads_ 是bimap
         *bimap<size_t, std::string>
         * 相当于两个不同方向的std::map，两个视图都是key -> value的数据结构，也称为左视图和右视图。
         * */
        auto completed_read = completed_reads_.left.find(clk());//找到当前时刻待处理的包
        //双向map left就是key找value，right就是value找key
        if (completed_read != completed_reads_.left.end()) {
            const std::string& key = completed_read->second;//left.second就是得到value   right.second就是得到key？
            std::list<utils::Packet>& queue = packet_queues_.at(key);//获取当前key待处理的队列     就是在此前，已经来过好几个对于key的请求了，一直都没处理，都是在
            //z时间之内的，所以这次可以直接处理好几个请求
            assert(!queue.empty()); // Sanity check: Queue may not be empty

            // Fetch the cache set corresponding to this key
            size_t cache_idx = getCacheIndex(key);//key有对应的cache号，里面有hash什么的
            BaseCacheSet& cache_set = *cache_sets_[cache_idx];

            // Sanity checks
            assert(!cache_set.contains(key));
            assert(queue.front().getTotalLatency() == kCacheMissLatency);
            std::cout<<"process_all writeq"<<std::endl;
            // Commit the queued entries
            cache_set.writeq(queue);//处理队列  主要就是用write去调用的各种算法  虚函数  继承后实现
            //processed_packets是输入的

            processed_packets.insert(processed_packets.end(),
                                     queue.begin(), queue.end());

            // Purge the queue, as well as the bidict mapping  清除队列以及bidict映射
            queue.clear();
            packet_queues_.erase(key);
            completed_reads_.left.erase(completed_read);
        }

        // Finally, increment the clock
        incrementClk();//重要！
    }

    /**
     * Processes the parameterized packet.
     * the first parameter is a packet for process,the second parameter is the all packets for record the information
     * 第一个参数是当前处理的这个包，第二个参数的是要存储所有包的信息，是个list
     *
     *      */
    void process(utils::Packet& packet, std::list<utils::Packet>& processed_packets) {
        packet.setArrivalClock(clk());
        //std::cout<<"clk:arrival"<<clk()<<std::endl;//debug here
        const std::string& key = packet.getFlowId();//key  i.e. flow_id
        auto queue_iter = packet_queues_.find(key);

        auto cacheindex=getCacheIndex(key);
        BaseCacheSet& cache_set = *cache_sets_.at(cacheindex);//返回缓存序号
        //std::cout<<"cache index:"<<cacheindex<<std::endl;
        // according to the key/flowid find the cache_set


        // Record arrival of the packet at the cache and cache-set levels.
        recordPacketArrival(packet);
        cache_set.recordPacketArrival(packet);

     // If this packet corresponds to a new flow, allocate its context
     //如果这个包对应于一个新的流，分配它的上下文
        if (memory_entries_.find(key) == memory_entries_.end()) {
            assert(!cache_set.contains(key));
            memory_entries_.insert(key);
            // Assume that insertions have zero cost.Insert the new entry into the cache.假设插入具有零成本。将新条目插入缓存。
            //但是一般kIsPenalizeInsertions=true啊
//            if (!kIsPenalizeInsertions) {
//                cache_set.write(key, packet);
//            }
        }
        // First, if the flow is cached, process the packet immediately.
        // This implies that the packet queue must be non-existent.
        //如果这个flow已经被缓存过了且不用继续排队，立即处理
        if (cache_set.contains(key)) {
            assert(queue_iter == packet_queues_.end());

            // Note: We currently assume a
            // zero latency cost for hits.

            cache_set.write(key, packet);//会有替换和更新LRU队列的过程

            packet.finalize();
            processed_packets.push_back(packet);
            total_latency_ += packet.getTotalLatency();
           std::cout<<"if (cache_set.contains(key))"<<packet.getTotalLatency()<<std::endl;//
        }
        // Else, we must either: a) perform a blocking read from memory,
        // or b) wait for an existing blocking read to complete. Insert
        // this packet into the corresponding packet queue.
        //否则，我们必须从内存执行阻塞读取，或等待现有的阻塞读取完成。将此数据包插入相应 的数据包队列。
        else {
            // If this flow's packet queue doesn't yet exist, this is the
            // blocking packet, and its read completes on cycle (clk + z).
            if (queue_iter == packet_queues_.end()) {//这个流没出现过
                size_t target_clk = clk() + kCacheMissLatency -1;
                //这个流没出现过,最快也是在target_clk后才能缓存过来，kCacheMissLatency=z
                //kCacheMissLatency就是z
                //std::cout<<"clk:"<<clk()<<" kCacheMissLatency"<<kCacheMissLatency<<std::endl;//10
                assert(completed_reads_.right.find(key) == completed_reads_.right.end());
                assert(completed_reads_.left.find(target_clk) == completed_reads_.left.end());

                completed_reads_.insert(boost::bimap<size_t, std::string>::value_type(target_clk, key));//插入阻塞的，target_clk和待处理的key
                packet.addLatency(kCacheMissLatency);
                std::cout<<"packet.addLatency(kCacheMissLatency);"<<kCacheMissLatency<<std::endl;//
                cache_set.writehalf(key,packet);
                packet.finalize();

                // Initialize a new queue for this flow
                packet_queues_[key].push_back(packet);
            }
            // Update the flow's packet queue
            else {//如果队列存在
                size_t target_clk = completed_reads_.right.at(key);
                packet.setQueueingDelay(queue_iter->second.size());//排队延迟
                packet.addLatency(target_clk - clk() + 1);
                packet.finalize();

                // Add this packet to the existing flow queue
                queue_iter->second.push_back(packet);
            }
            assert(packet.isFinalized()); // Sanity check,確保當前packet完成
            total_latency_ += packet.getTotalLatency();
        }
        // Process any completed reads，再把未处理完成的处理一下
        processAll(processed_packets);
    }

    /**
     * Indicates completion of the warmup period.
     */
    void warmupComplete() {
        total_latency_ = 0;
        packet_queues_.clear();
        completed_reads_.clear();
    }

    /**
     * Indicates completion of the simulation.
     * 表示模拟完成。
     * 拆卸
     */
    void teardown(std::list<utils::Packet>& processed_packets) {
        while (!packet_queues_.empty()) {
            processAll(processed_packets);
        }
    }

    /**
     * Save the raw packet data to file.
     */


    /**
   * Save the raw packet data to file. 
   */
    static void savePackets(std::list<utils::Packet>& packets,
                            const std::string& packets_fp) {
        if (!packets_fp.empty()) {
            std::ofstream file(packets_fp, std::ios::out |
                                           std::ios::app);
            // Save the raw packets to file
            for (const utils::Packet& packet : packets) {

                std::cout << packet.getFlowId() << ";";
               // std::cout << static_cast<size_t>(packet.getTotalLatency()) << ";"<< static_cast<size_t>(packet.getQueueingDelay()) << std::endl;
            }
        }
        packets.clear();
    }



    /**
     * Generate and output model benchmarks.
     */
    static void benchmark(BaseCache& model, const std::string& trace_fp, const std::
                          string& packets_fp, const size_t num_warmup_cycles) {
        std::list<utils::Packet> packets; // List of processed packet
        //utils::Packet
        /*
         *  const std::string flow_id_;

    // Housekeeping
    size_t arrival_clock_ = 0;
    double total_latency_ = 0;
    double queueing_delay_ = 0;
    bool is_finalized_ = false;
         *
         * */
        size_t num_total_packets = 0; // Total packet count
        if (!packets_fp.empty()) {//if the path is not empty,because this is optional

            std::ofstream file(packets_fp, std::ios::out |
                                           std::ios::trunc);
            // Write the header

            file << model.name() <<  ";"
                 << model.kMaxNumCacheSets << ";" << model.kMaxNumCacheEntries
                 << std::endl;
        }
        // Process the trace
        std::string line;
        std::ifstream trace_ifs(trace_fp);
        while (std::getline(trace_ifs, line)) {
            //对于每一行
            std::string timestamp, flow_id;

            /****************************************
             * Important note: Currently, we ignore *
             * packet timestamps and inject at most *
             * 1 packet into the system each cycle. *
             ****************************************/

            // Nonempty packet
            if (!line.empty()) {
                std::stringstream linestream(line);

                // Parse the packet's flow ID and timestamp
                std::getline(linestream, timestamp, ';');
                std::getline(linestream, flow_id, ';');
            }
            // Cache warmup completed
//            if (num_total_packets == num_warmup_cycles) {
//                model.warmupComplete(); packets.clear();
//                std::cout << "> Warmup complete after "
//                         << num_warmup_cycles
//                          << " cycles." << std::endl;//最初的周期是warmup周期，不能处理数据包，故需要计数后重新再开始
//            }
            // Periodically save packets to file，
            if (num_total_packets > 0 &&
                num_total_packets % 5000000 == 0) {
                if (num_total_packets >= num_warmup_cycles) {
                    savePackets(packets, packets_fp);
                    std::cout<<"print periodically"<<std::endl;
                }
                std::cout << "On packet: " << num_total_packets
                          << ", latency: " << model.getTotalLatency() << std::endl;
            }
            // Process the packet
            if (!flow_id.empty()) {
                std::cout<<"----------------"<<std::endl;
                std::cout<<"current timestamp:"<< timestamp <<std::endl;
                std::cout<<"init"<<flow_id<<std::endl;
                utils::Packet packet(flow_id);
                model.process(packet, packets);
            }
            else { model.processAll(packets); }
            num_total_packets++;
        }
        model.teardown(packets);
        savePackets(packets, packets_fp);

        // Perform teardown



        // Simulations results
        size_t total_latency = model.getTotalLatency();
        double average_latency = (
            (num_total_packets == 0) ? 0 :
            static_cast<double>(total_latency) / num_total_packets);

        // Debug: Print trace and simulation statistics
        std::cout << std::endl;
        std::cout << "Total number of packets: " << num_total_packets << std::endl;
        std::cout << "Total latency is: " << model.getTotalLatency() << std::endl;
        std::cout << "Average latency is: " << std::fixed << std::setprecision(2)
                 << average_latency << std::endl << std::endl;
    }

    /**
     * Run default benchmarks.
     */
    template<class T>
    static void defaultBenchmark(int argc, char** argv) {
        using namespace boost::program_options;
        std::ofstream outfile;
        //outfile.open("0629.txt");

        // Parameters
        size_t z;
        double c_scale;
        double  cache_size ;
        double num_cache_sets;
        std::string trace_fp;
        std::string packets_fp;
        size_t set_associativity;
        size_t num_warmup_cycles;

        // Program options
        variables_map variables;
        options_description desc{"A caching simulator that models Delayed Hits"};

        try {//其实只需要  trace，cachesize，zfactor（甚至不需要），
            // Command-line arguments
            desc.add_options()
                    ("help",        "Prints this message")
                    ("trace",value<std::string>(&trace_fp)->required(),"Input trace file path")
                    ("k",value<double>(&cache_size)->required(),"Parameter,k,cache size")
                    ("zfactor",value<size_t>(&z)->required(),"Parameter,z,zfactor")

                    ("cscale",value<double>(&c_scale)->default_value(2),"Parameter: Cache size (%Concurrent Flows)")
                    ("packets",     value<std::string>(&packets_fp)->default_value(""),   "[Optional] Output packets file path")
                    ("associativity",         value<size_t>(&set_associativity)->default_value(0),  "[Optional] Parameter: Cache set-associativity")
                    ("warmup",      value<size_t>(&num_warmup_cycles)->default_value(0),  "[Optional] Parameter: Number of cache warm-up cycles");

            // Parse model parameters
            store(parse_command_line(argc, argv, desc), variables);

            // Handle help flag
            if (variables.count("help")) {
                std::cout << desc << std::endl;
                return;
            }
            store(command_line_parser(argc, argv).options(
                desc).allow_unregistered().run(), variables);
            notify(variables);
        }
        // Flag argument errors
        catch(const required_option& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return;
        }
        catch(...) {
            std::cerr << "Unknown Error." << std::endl;
            return;
        }


        if (set_associativity == 0) { set_associativity = std::max<size_t>(
            1, static_cast<size_t>(round(cache_size)));
        }




        num_cache_sets=1;
        // Debug: Print the cache and trace parameters

        std::cout << "CacheNumbers" << num_cache_sets <<std::endl;
        std::cout << "Parameters: k=" << cache_size <<std::endl;
        std::cout << "Parameters: z=" << z<<std::endl;


        // Instantiate the model
        T model(z, set_associativity, num_cache_sets, true,
                HashType::MURMUR_HASH, argc, argv);

        // Debug: Print the model parameters
        std::cout << model.name()
                  <<   ", kMaxNumCacheSets=" << model.kMaxNumCacheSets << ", kMaxNumCacheEntries="
                  << model.kMaxNumCacheEntries << std::endl;

        std::cout << "Starting trace " << trace_fp <<  "..." << std::endl << std::endl;
        BaseCache::benchmark(model, trace_fp,  packets_fp, num_warmup_cycles);
    }
};

} // namespace caching

#endif // cache_base_h
