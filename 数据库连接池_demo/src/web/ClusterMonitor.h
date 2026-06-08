#ifndef CLUSTER_MONITOR_H
#define CLUSTER_MONITOR_H

#include <string>
#include <map>
#include <thread>
#include <atomic>
#include <vector>

#include "../RedisConn.h"

namespace WebServer {

struct NodeStatus {
    int port;
    bool online;
    int mysql_active;
    int mysql_idle;
    int mysql_total;
    int mysql_maxActive;
    int redis_active;
    int redis_idle;
    int redis_total;
    int redis_maxActive;
    std::string timestamp;
};

struct ClusterStatus {
    int total_nodes;
    int online_nodes;
    int offline_nodes;
    int total_mysql_active;
    int total_mysql_idle;
    int total_mysql_max;
    int total_redis_active;
    int total_redis_idle;
    int total_redis_max;
    std::map<int, NodeStatus> nodes;
};

class ClusterMonitor {
public:
    static ClusterMonitor& instance();
    
    bool init(int port, const std::string& redisHost = "localhost", int redisPort = 6379);
    void startStatusReporter();
    void stopStatusReporter();
    
    // 动态节点管理
    void registerNode();
    void unregisterNode();
    std::vector<int> getRegisteredPorts();
    
    // 集群配置同步
    void broadcastConfigUpdate(const std::string& poolType, int maxActive, int minIdlePercent, int maxWaitMs, int idleTimeoutMs);
    void loadInitialConfig();
    bool checkAndApplyRemoteConfig();
    
    ClusterStatus getClusterStatus();
    NodeStatus getLocalNodeStatus();
    
private:
    ClusterMonitor();
    ~ClusterMonitor();
    
    void reportStatusLoop();
    void writeLocalStatus();
    void cleanupExpiredNodes(RedisConn& conn);
    std::string getNodeKey(int port);
    
    int m_port;
    std::string m_redisHost;
    int m_redisPort;
    std::thread* m_reporterThread;
    std::atomic<bool> m_running;
    std::string m_lastMysqlVersion;
    std::string m_lastRedisVersion;
};

}

#endif