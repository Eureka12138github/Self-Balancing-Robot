#include "cbuf_slot.h"  


// 环形缓冲区结构体的隐藏定义
struct circular_buf_t {
    uint8_t* buffer;      ///< 用户提供的数据存储区（不归本库管理）
    size_t write_index;   ///< 写指针：下一个要写入的位置
    size_t read_index;    ///< 读指针：下一个要读取的位置
    size_t max;           ///< 缓冲区最大容量（元素个数）
};

static circular_buf_t g_cbuf_pool[CBUF_MAX_INSTANCES] = {0};
static u8 g_cbuf_count = 0;  // 已分配数量



/**
 * @brief 获取当前已分配的环形缓冲区实例数量
 *
 * 此函数返回由 circular_buf_init() 成功创建且尚未释放（本实现为静态池，无显式释放）的
 * 环形缓冲区句柄总数。该值单调递增，最大不超过 CBUF_MAX_INSTANCES。
 *
 * @return 当前已分配的实例数（范围：0 ～ CBUF_MAX_INSTANCES）
 *
 * @note
 * - 该函数可用于资源监控或判断是否还能创建新实例；
 * - 返回值在多任务环境下非原子，仅作参考。
 */
uint8_t circular_buf_get_instance_count(void) {
    return g_cbuf_count;
}

/**
 * @brief 静态初始化环形缓冲区对象
 * 
 * 使用用户提供的内存缓冲区创建一个新的环形缓冲区实例。
 * 该函数从预分配的缓冲池中获取一个可用的环形缓冲区对象，
 * 并将其与用户提供的缓冲区内存关联。
 * 
 * @param[in] buffer 用户提供的数据存储缓冲区，不能为空指针
 * @param[in] size   缓冲区大小（字节数），必须大于0
 * 
 * @return 成功时返回环形缓冲区句柄，失败时返回NULL
 *         失败原因可能包括：
 *         - buffer 为 NULL
 *         - size 为 0
 *         - 全局实例池已满（超过 CBUF_MAX_INSTANCES 限制）
 * 
 * @note 
 *    - 该函数采用防御性编程策略，同时使用断言（开发期）和运行时检查（生产期）
 *    - CBUF_ASSERT 在开发阶段用于快速发现编程错误
 *    - 在生产环境中，函数会返回 NULL 而不是崩溃
 *    - 该函数假定在单线程环境中调用（实例池管理非线程安全）
 *    - 调用成功后，缓冲区将被重置为初始状态（空状态）
 *
 * @warning 
 *   - 本函数非线程安全；
 *   - 多任务/中断环境下调用前必须加锁（如禁用中断、获取 mutex）。
 * 
 * @par 使用示例：
 * @code
 *     uint8_t my_buffer[256];
 *     cbuf_handle_t handle = circular_buf_init(my_buffer, sizeof(my_buffer));
 *     if (handle != NULL) {
 *         // 使用缓冲区...
 *     } else {
 *         // 处理初始化失败
 *     }
 * @endcode
 * 
 * @pre 
 *    - buffer 指向的内存区域必须至少有 size 字节的可用空间
 *    - 调用此函数的上下文必须保证单线程访问（如果需要多线程安全，请在外部加锁）
 * 
 * @post 
 *    - 如果返回非NULL，缓冲区对象已正确初始化并处于空状态
 *    - 全局实例计数器 g_cbuf_count 已增加
 * 
 * @see circular_buf_reset
 * @see CBUF_MAX_INSTANCES
 */
cbuf_handle_t circular_buf_init(uint8_t* buffer, size_t size) 
{
    // 开发期断言检查
    CBUF_ASSERT(buffer && size > 0);
    
    // 生产期运行时检查
    if (!buffer || size == 0) {
        return NULL;  // 参数无效
    }
    
    // 检查实例池是否已满
    if (g_cbuf_count >= CBUF_MAX_INSTANCES) {
        return NULL;     // 生产期处理
    }
    
    circular_buf_t* cbuf = &g_cbuf_pool[g_cbuf_count++];
    cbuf->buffer = buffer;
    cbuf->max = size;
    circular_buf_reset(cbuf);
    return cbuf;
}

/**
 * @brief 重置缓冲区至空状态
 *
 * 丢弃所有数据，恢复初始状态。
 *
 * @param[in] me 有效句柄（非 NULL）
 *
 * @note 不修改用户 \p buffer 内容，仅重置控制状态。
 * @warning 非线程安全。共享缓冲区时需互斥访问。
 */
void circular_buf_reset(cbuf_handle_t me) {
    CBUF_ASSERT(me);
    me->read_index = 0;
    me->write_index = 0;
}


/**
 * @brief 检查缓冲区是否已满
 *
 * @param[in] me 有效句柄（非 NULL）
 * @return true 表示满，false 表示可写入
 * @note 无副作用，但结果可能立即失效（多任务场景）。
 */
bool circular_buf_full(cbuf_handle_t me) {
    CBUF_ASSERT(me);
	size_t head = me->write_index + 1;
	 if(head == me->max) {
		head = 0; 
	 } 
	return head == me->read_index;
}

/**
 * @brief 检查缓冲区是否为空
 *
 * @param[in] me 有效句柄（非 NULL）
 * @return true 表示空，false 表示可读取
 * @note 依赖 \c full 标志准确区分空/满状态。
 */
bool circular_buf_empty(cbuf_handle_t me) {
    CBUF_ASSERT(me);
	return (me->write_index == me->read_index);
}

/**
 * @brief 获取缓冲区最大容量
 *
 * @param[in] me 有效句柄（非 NULL）
 * @return 缓冲区总容量（恒定不变）
 * @note 无副作用，常用于与当前大小比较。
 */
size_t circular_buf_capacity(cbuf_handle_t me) {
    CBUF_ASSERT(me);
    return me->max - 1;
}

/**
 * @brief 获取缓冲区当前存储的元素数量
 *
 * 计算有效数据个数，范围 [0, capacity-1]。
 *
 * @param[in] me 有效句柄（非 NULL）
 * @return 当前元素数量
 *
 * @note
 * - 无副作用；
 * - 若缓冲区满，直接返回 \c max；
 * - 否则根据 read/write 指针相对位置分连续或绕回两种情况计算。
 * @warning 非原子操作，多任务环境下结果可能立即失效。
 */
size_t circular_buf_size(cbuf_handle_t me) {
    CBUF_ASSERT(me);
    
    if (circular_buf_full(me)) {
        return me->max - 1;  // slot 版本最大容量
    }
    
    if (me->write_index >= me->read_index) {
        return me->write_index - me->read_index;
    } else {
        // 绕回情况：[read, max) + [0, write)
        return (me->max - me->read_index) + me->write_index;
    }
}



/**
 * @brief 向环形缓冲区写入一个字节（非覆盖模式）
 *
 * 仅在缓冲区未满时写入数据。若缓冲区已满或参数无效，则写入失败。
 *
 * @param[in,out] me    环形缓冲区句柄（必须由 circular_buf_init 初始化）
 * @param[in]     data  要写入的字节
 *
 * @return 
 *   - `0`：成功写入；
 *   - `-1`：失败（原因：句柄为 NULL、内部 buffer 为 NULL，或缓冲区已满）。
 *
 * @note
 *   - 本实现基于 **slot 版本**（牺牲一个存储单元以区分空/满状态）；
 *   - 缓冲区实际容量为 `max - 1`；
 *   - 写入成功后，内部 write_index 自动前移并处理回绕；
 *   - 此函数为 **非覆盖模式**：满时拒绝写入，不会丢弃已有数据。
 *
 * @warning
 *   - 非线程安全：多任务/中断环境下需外部加锁；
 *   - 不适用于需要“最新数据优先”的场景（如实时采样），此类场景应使用 full-flag 覆盖版本。
 *
 * @see circular_buf_get, circular_buf_full, circular_buf_empty
 */
int8_t  circular_buf_put(cbuf_handle_t me, uint8_t data) {        
    CBUF_ASSERT(me && me->buffer);
    if (!me || circular_buf_full(me)) {
        return -1;
    } 
    me->buffer[me->write_index] = data;
    if (++(me->write_index) == me->max) {
        me->write_index = 0;
    }
    return 0;    
}


/**
 * @brief 从环形缓冲区获取一个字节的数据
 * 
 * 该函数从环形缓冲区的读指针位置读取一个字节的数据，并将读指针向前移动一位。
 * 如果读指针到达缓冲区末尾，则自动回绕到缓冲区开头。
 * 
 * @param[in]  me   环形缓冲区句柄，不能为空指针
 * @param[out] data 指向存储读取数据的目标缓冲区，不能为空指针
 *                 函数成功执行后，该地址将保存从缓冲区读取的字节数据
 * 
 * @return int 返回操作结果状态
 *             0    : 成功读取数据
 *            -1    : 操作失败，可能的原因：
 *                    - 参数无效（me为NULL或data为NULL）
 *                    - 缓冲区为空（没有可读数据）
 * 
 * @note 
 *    - 该函数采用防御性编程策略，同时使用断言（开发期）和运行时检查（生产期）
 *    - CBUF_ASSERT宏用于开发阶段检测编程错误，发布版本中通常被禁用
 *    - 在单生产者单消费者场景下可无锁使用（非通用线程安全）
 *    - 读指针会在到达缓冲区边界时自动回绕
 * 
 */
int8_t   circular_buf_get(cbuf_handle_t me, uint8_t* data) 
{
    CBUF_ASSERT(me && data && me->buffer);  // 开发期检查
    
    // 生产期检查
    if(!me || !data || circular_buf_empty(me)) {
        return -1;  // 或其他错误码
    }
    
    *data = me->buffer[me->read_index];
    if(++(me->read_index) == me->max) {
        me->read_index = 0;
    }
    
    return 0;  // 成功
}

/**
 * @brief 从环形缓冲区 peek 多个字节（不移动读指针）
 *
 * 将缓冲区中从读指针开始的最多 len 个字节复制到 dst 中，
 * 不改变 read_index。
 *
 * @param[in]  me   有效句柄
 * @param[out] dst  目标缓冲区（必须足够大）
 * @param[in]  len  请求 peek 的字节数
 * @return 实际 peek 的字节数（<= len）
 */
size_t circular_buf_peek(cbuf_handle_t me, uint8_t* dst, size_t len) {
    CBUF_ASSERT(me && dst);
    if (!me || !dst) return 0;

    size_t available = circular_buf_size(me);
    if (available == 0) return 0;
	
    size_t to_copy = (len < available) ? len : available;
    size_t copied = 0;
    size_t ridx = me->read_index;
	
    while (copied < to_copy) {
        dst[copied++] = me->buffer[ridx];
        if (++ridx == me->max) ridx = 0;
    }

    return copied;
}

/**
 * @brief 从环形缓冲区跳过（丢弃）指定数量的字节
 *
 * @param[in] me   有效句柄
 * @param[in] len  要跳过的字节数
 * @return 实际跳过的字节数（<= len，且 <= 当前 size）
 */
size_t circular_buf_skip(cbuf_handle_t me, size_t len) {
    CBUF_ASSERT(me);
    if (!me) return 0;

    size_t available = circular_buf_size(me);
    if (available == 0) return 0;
	//这里保证不会超写指针
    size_t to_skip = (len < available) ? len : available;
    me->read_index += to_skip;
    if (me->read_index >= me->max) {
        me->read_index -= me->max; // 支持 skip 跨越边界
    }
    return to_skip;
}



