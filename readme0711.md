note:
    cache_size与associate的大小是有关的，其实初始化时只要不指定associate，的大小也就是cache_size的大小了

1. 从cpp文件中的main函数BaseCache::defaultBenchmark<LRUCache>(argc, argv);

2. 调用到cache_base中的defaultBenchmark

   1. defaultBenchmark做了什么？

      接收各种参数

      计算max_num_concurrent_flows，不过在我们的模型里面没什么用

      **我们的模型里比较重视的两个参数是num_cache_sets=1，cache_size=k**，z要把代码改一下，执行命令的时候，输入k和z即可？

      开始模拟，BaseCache::**benchmark**(model, trace_fp, packets_fp, num_warmup_cycles);其中的参数，model指的是当前使用的算法，trace_fp是输入数据集，后面两个参数没什么用

3. benchmark()

   ```c++
   static void benchmark(BaseCache& model, const std::string& trace_fp, const std::
                         string& packets_fp, const size_t num_warmup_cycles)
   ```

   - 流程

     * 按行读入packets_fp中的输入数据集，获取到timestamp和flow_id
* 等待缓存预热、每5000000行保存一下输出数据（这好像都没什么用）
  
* process，真正的开始处理
  
* 如果文件读入完毕  
  
```c++
     model.processAll 
     model.teardown(packets);//其实也是调用processAll
     savePackets(packets, packets_fp);
```

* 最后输出total latency和平均latency
  
- 变量
  
  ​	std::list\<utils::Packet\> packets;    Packet的list Include the all infomation about the packet   list包含重复的
  
  ​	utils::Packet packet(flow_id);   每到达的一行 都形成一个packet
  
4. process

    ```
    void process(utils::Packet& packet, std::list<utils::Packet>& processed_packets)
    ```

    第一个参数是当前处理的这个包，第二个参数的是要存储所有包的信息，是个list

    * 流程
      * 获取当前packet的flow_id、位于哪个缓存，当然，本模型中只有一个缓存 cacheindex=1，根据flow_id获取queue_iter（迭代器）

      * 记录packet到达

      * 如果这个包对应于一个新的流，分配它的上下文，memory_entries_是一个无序的集合，存储曾经来过的流，如果kIsPenalizeInsertions为false 的话，也就是说，曾经来过的流就可以无成本的插入，那么这个过程就是有用的，但是我们的模型中kIsPenalizeInsertions一般认为是true的，所以这儿也没什么用

      * 如果请求的东西已经在缓存中

        if (cache_set.contains(key)) {
            assert(queue_iter == packet_queues_.end());
            cache_set.write(key, packet);//会有替换及更新LRU队列的过程
            packet.finalize();
            processed_packets.push_back(packet);//将当前包加入已处理list
            total_latency_ += packet.getTotalLatency();
           std::cout<<"if (cache_set.contains(key))"<<packet.getTotalLatency()<<std::endl;//
        }

      * 如果请求的东西还不在缓存中

        *  如果不存在于正在请求该文件的队列

        		1. target_clk,到达target_clk，文件才能回来，这里有问题，他这到了target_clk才驱逐
          		2. 向completed_reads_这个bimap中，插入target_clk和key(flow_id)_
          		3. _将packet加入packet_queues_[key]这个list中，待处理队列

        * 如果存在于正在请求的该文件的队列中
          * 找到target_clk
          * 增加延迟，target_clk - clk() + 1
          * 在packet_queues_的迭代器queue_iter中添加packet

      processAll()

      model.teardown(packets);//如果还有没处理干净的包，再处理一下
       savePackets(packets, packets_fp);d

* 变量

  * packet_queues_  ，std::unordered_map<std::string, std::list\<utils::Packet\>\>
  * BaseCacheSet& **cache_set** = *cache_sets_.at(cacheindex)， 返回一个缓存，一个BaseCacheSet里面有多个CacheEntry     也就是一个缓存有几个slot    cachesize_
  * _std::unordered_set\<std::string\> memory_entries_  // Set of keys in the global store
  * completed_reads_    boost::bimap<size_t, std::string>  双向map
  * processed_packets  感觉就没什么用   输出的时候能用到   
* 函数

5. processAll
   * 流程
     * 根据当前clk找到待处理的数据包们，返回的completed_read里面应该有待处理的包们
     * 






