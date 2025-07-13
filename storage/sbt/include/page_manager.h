#ifndef PAGE_MANAGER_H
#define PAGE_MANAGER_H

#include "sbt_common.h"
#include <fstream>
#include <unordered_set>

namespace sbt {

class PageManager {
public:
    PageManager(const std::string& filename);
    ~PageManager();
    
    // 页面分配和释放
    PageId allocate_page();
    void deallocate_page(PageId page_id);
    
    // 页面读写
    bool read_page(PageId page_id, char* buffer);
    bool write_page(PageId page_id, const char* buffer);
    bool sync_page(PageId page_id);
    
    // 批量操作
    bool read_pages(const std::vector<PageId>& page_ids, 
                   std::vector<char*>& buffers);
    bool write_pages(const std::vector<PageId>& page_ids,
                    const std::vector<const char*>& buffers);
    
    // 文件管理
    bool open_file();
    bool close_file();
    bool sync_all();
    bool truncate_file();
    
    // 统计信息
    size_t get_total_pages() const;
    size_t get_free_pages() const;
    size_t get_used_pages() const;
    size_t get_file_size() const;
    
    // 页面验证
    bool validate_page(PageId page_id) const;
    bool is_page_allocated(PageId page_id) const;

private:
    // 内部辅助函数
    bool load_free_page_list();
    bool save_free_page_list();
    bool extend_file(size_t new_page_count);
    off_t get_page_offset(PageId page_id) const;
    
    // 页面头部管理
    struct PageHeader {
        uint32_t magic_number;
        uint32_t checksum;
        PageId page_id;
        uint32_t page_type;
        uint32_t data_size;
        uint64_t timestamp;
    };
    
    bool write_page_header(PageId page_id, const PageHeader& header);
    bool read_page_header(PageId page_id, PageHeader& header);
    uint32_t calculate_checksum(const char* data, size_t size);

private:
    std::string filename_;
    std::fstream file_;
    bool file_opened_;
    
    // 空闲页面管理
    std::unordered_set<PageId> free_pages_;
    PageId next_page_id_;
    
    // 文件元数据
    size_t total_pages_;
    size_t file_size_;
    
    // 同步控制
    mutable std::shared_mutex page_mutex_;
    
    // 常量
    static constexpr uint32_t MAGIC_NUMBER = 0x53425400; // "SBT\0"
    static constexpr size_t HEADER_SIZE = sizeof(PageHeader);
    static constexpr size_t DATA_SIZE = PAGE_SIZE - HEADER_SIZE;
};

} // namespace sbt

#endif // PAGE_MANAGER_H
