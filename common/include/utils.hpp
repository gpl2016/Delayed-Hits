#ifndef utils_hpp
#define utils_hpp

// STD headers
#include <algorithm>
#include <array>
#include <assert.h>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// Boost headers
#include <boost/functional/hash.hpp>

// Macros
#define SUPPRESS_UNUSED_WARNING(a) ((void) a)

namespace utils {

// Helper functions
bool DoubleApproxEqual(const double a, const double b,
                       const double epsilon=1e-6) {
    return fabs(a - b) < epsilon;
}

bool DoubleApproxGreaterThanOrEqual(const double a, const double b,
                                    const double epsilon=1e-6) {
    return (a > b) || DoubleApproxEqual(a, b, epsilon);
}

/**
 * Represents a network packet.
 * Packet->Include the all infomation about the packet
 *  */
class Packet {
private:
    // Flow/Object identifier
    const std::string flow_id_;

    // Housekeeping
    size_t arrival_clock_ = 0;
    double total_latency_ = 0;
    double queueing_delay_ = 0;
    bool is_finalized_ = false;

public:
    Packet(const std::string& flow_id) : flow_id_(flow_id) {}

    // Error-handling
    inline bool checkNotFinalized() {
        if (is_finalized_) { throw std::runtime_error(
            "Cannot modify a finalized packet.");
        }
        return !is_finalized_;
    }

    // Accessors
    bool isFinalized() const { return is_finalized_; }
    const std::string& getFlowId() const { return flow_id_; }
    double getTotalLatency() const { return total_latency_; }
    double getQueueingDelay() const { return queueing_delay_; }
    size_t getArrivalClock() const { return arrival_clock_; }

    // Mutators
    void addLatency(const double value) {
        if (checkNotFinalized()) { total_latency_ += value; }
    }
    void incrementLatency() { addLatency(1.); }
    void setQueueingDelay(const double delay) {
        if (checkNotFinalized()) { queueing_delay_ = delay; }
    }
    void setArrivalClock(const size_t clock) {
        if (checkNotFinalized()) { arrival_clock_ = clock; }
    }
    void finalize() {
        if (checkNotFinalized()) { is_finalized_= true; }
    }
};

/**
 * Container for flow-related data.
 */
class FlowData {
private:
    std::string protocol_; // (Currently unused)
    size_t size_in_bytes_ = 0; // (Currently unused)
    std::list<size_t> indices_; // List of packet indices

public:
    // Accessors
    size_t sizeInBytes() const { return size_in_bytes_; }
    const std::string& protocol() const { return protocol_; }
    const std::list<size_t>& indices() const { return indices_; }
    size_t getIdxRange() const { return indices_.empty() ? 0 :
                                        (indices_.back() - indices_.front()); }
    // Mutators
    void addPacket(const size_t idx) { indices_.push_back(idx); }
    void incrementFlowSize(const size_t size) { size_in_bytes_ += size; }
    void setProtocol(const std::string& protocol) { protocol_ = protocol; }
};

/**
 * Given the path to a trace file, returns
 * a string vector representing the trace.
 */
std::vector<std::string>
parseTrace(const std::string& trace_fp) {
    std::ifstream trace_ifs(trace_fp);
    std::vector<std::string> trace;
    std::string line;

    // Populate the trace vector
    while (std::getline(trace_ifs, line)) {
        std::string timestamp, flow_id;

        // Nonempty packet
        if (!line.empty()) {
            std::stringstream linestream(line);

            // Parse the packet's timestamp and flow ID
            std::getline(linestream, timestamp, ';');
            std::getline(linestream, flow_id, ';');
        }
        trace.push_back(flow_id);
    }
    return trace;
}

/**
 * Implements an analyzer for a packet trace.
 */
class TraceAnalyzer {
private:
    const std::string trace_fp_; // Path to trace file
    size_t num_total_packets_ = 0; // Total packet count
    std::unordered_map<std::string, FlowData> flow_ids_to_data_map_;

    /**
     * Internal helper method. Generates a mapping between flow
     * IDs and the relevant flow metadata for the given trace.
     */
    void analyzeFlowArrivals() {
        std::ifstream trace_ifs(trace_fp_);

        // Populate the trace vector
        std::string line;
        while (std::getline(trace_ifs, line)) {

            // Non-empty packet
            if (!line.empty()) {
                std::string timestamp, flow_id;
                std::stringstream linestream(line);

                // Parse the packet's timestamp and flow ID
                std::getline(linestream, timestamp, ';');
                std::getline(linestream, flow_id, ';');

                // Update the corresponding flow data
                FlowData& flow_data = flow_ids_to_data_map_[flow_id];
                flow_data.addPacket(num_total_packets_);
            }
            // Update the total packet count
            num_total_packets_++;
        }
    }

public:
    TraceAnalyzer(const std::string& trace_fp) :
    trace_fp_(trace_fp) { analyzeFlowArrivals(); }

    // Accessors
    size_t getNumPackets() const { return num_total_packets_; }
    size_t getNumFlows() const { return flow_ids_to_data_map_.size(); }
    const FlowData& getFlowData(const std::string& flow_id) const {
        return flow_ids_to_data_map_.at(flow_id);
    }
    const std::unordered_map<std::string, FlowData>&
    getFlowIdsToDataMap() const { return flow_ids_to_data_map_; }
};

/**
 * Container storing trace metadata.
 */
struct TraceMetadata {
    size_t num_total_flows = 0;
    size_t num_concurrent_flows = 0;
};

/**
 * Helper method.
 *
 * Given a map of flow IDs and their idx ranges, returns the total number of
 * flows, as well as the maximum number of concurrent flows at any timestep.
 */
TraceMetadata getFlowCounts(const std::unordered_map<std::string,std::pair<size_t, size_t>>& idx_ranges) {
        std::ofstream outputfile;
        outputfile.open ("idx_ranges.txt");
        for( const auto& n : idx_ranges) {
            //id_ranges : string,pair
            outputfile << "Key:[" << n.first << "] Value:[" << n.second.first <<" "<<n.second.second <<"]\n";
        }

    std::vector<std::pair<size_t, bool>> all_points;
    size_t global_max_num_concurrent_flows = 0;

    // Populate the points list
    for (const auto& element : idx_ranges) {
        const auto& idx_range = element.second;
        all_points.push_back(std::make_pair(idx_range.first, true));
        all_points.push_back(std::make_pair(idx_range.second, false));
    }
    //all_points里面是timestamp，某个flow的开始时间+true，结束时间+false

//        std::ofstream outputfile2;
//        outputfile2.open("all_points");
//    for(const auto & t: all_points){
//        outputfile2<<t.first<<" "<<t.second<<std::endl;
//    }

    // Next, sort the list (using the start/stop flag to break ties)
    //排序规则：all_points中timestamp升序，若timestamp相同，second=1优先
    std::sort(
        all_points.begin(), all_points.end(),
        [](const std::pair<size_t, bool>& left,
           const std::pair<size_t, bool>& right) {//
            return ((left.first < right.first) ||
                    ((left.first == right.first) &&
                      left.second && !right.second));
    });


    // Finally, compute the number of concurrent flows at every
    // timestep. If required, update the global maximum value.
    size_t local_max_num_concurrent_flows = 0;
    for (const auto& point : all_points) {//统计在5000个timestamps内，某时刻持续的最多的flow数
        // Started a new flow
        if (point.second) {//true->+1
            local_max_num_concurrent_flows++;
        }
        // Ended an existing flow
        else {//false->-1
            assert(local_max_num_concurrent_flows > 0);
            local_max_num_concurrent_flows--;
        }
        global_max_num_concurrent_flows=std::max(global_max_num_concurrent_flows,local_max_num_concurrent_flows);
//        if (local_max_num_concurrent_flows > global_max_num_concurrent_flows) {
//            global_max_num_concurrent_flows = local_max_num_concurrent_flows;
//        }
    }
    assert(local_max_num_concurrent_flows == 0); // Sanity check,finally local_max_num_concurrent_flows must equal to 0
    std::cout<<"196 exec"<<std::endl;
    return TraceMetadata{idx_ranges.size(), global_max_num_concurrent_flows};//what the getFlowCounts final returns
}

/**
 * Given a trace, returns the total number of flows as well
 * the maximum number of concurrent flows at any timestep.
 */
TraceMetadata getFlowCounts(const std::vector<std::string>& trace) {
    typedef std::pair<size_t, size_t> IdxRange; // [start, end]

    std::unordered_map<std::string, IdxRange> idx_ranges;
    for (size_t idx = 0; idx < trace.size(); idx++) {
        const std::string& flow_id = trace[idx];
        if (flow_id.empty()) { continue; }

        // This is the first request to this flow
        auto iter = idx_ranges.find(flow_id);
        if (iter == idx_ranges.end()) {
            idx_ranges[flow_id] = std::make_pair(idx, idx);
        }
        // Else, update the last idx
        else {
            iter->second.second = idx;
        }
    }
    // Invoke the helper method
        std::cout<<"245 exec"<<std::endl;
    return getFlowCounts(idx_ranges);
}

/**
 * Given the path to a trace file, returns the total number of flows
 * as well as the maximum number of concurrent flows at any timestep.
 */
TraceMetadata getFlowCounts(const std::string& trace_fp) {
    typedef std::pair<size_t, size_t> IdxRange; // [start, end]

    std::unordered_map<std::string, IdxRange> idx_ranges;
    std::ifstream trace_ifs(trace_fp);
    std::string line;
    size_t idx = 0;

    // Populate the trace vector
    while (std::getline(trace_ifs, line)) {//read to "line"

        // Nonempty packet
        if (!line.empty()) {
            std::string timestamp, flow_id;
            std::stringstream linestream(line);//stringstream,

            // Parse the packet's timestamp and flow ID
            std::getline(linestream, timestamp, ';');//stringstream,when readin the firsr part,stringstream will ignore the part
            //https://blog.csdn.net/weixin_43347376/article/details/105134445?utm_medium=distribute.pc_relevant.none-task-blog-baidujs_title-0&spm=1001.2101.3001.4242
            std::getline(linestream, flow_id,';');//"; or \n "
            // This is the first request to this flow
            auto iter = idx_ranges.find(flow_id);
            if (iter == idx_ranges.end()) {
                idx_ranges[flow_id] = std::make_pair(idx, idx);//idx actually is the timestamps
            }
            // Else, update the last idx
            else {
                iter->second.second = idx;//flow_id最开始请求的timestamps到最后请求的timestmaps
            }
        }
        idx++;
    }
    // Invoke the helper method
        std::cout<<"271 exec"<<std::endl;
    return getFlowCounts(idx_ranges);
}
} // namespace utils

// Hash tuples
namespace std
{
    template<typename... T>
    struct hash<tuple<T...>> {
        size_t operator()(tuple<T...> const& arg) const noexcept {
            return boost::hash_value(arg);
        }
    };
}

#endif // utils_hpp
