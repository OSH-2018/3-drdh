## 说明
#### 已实现
- 文件
  - 创建
  - 删除
  - 读
  - 写（可追加）
- 目录基本操作
  - 创建
  - 删除

#### 未实现
- 元数据修改
- cp 效率

#### 结构
- 文件和目录
  - 将文件和目录都视为节点
  - 目录不连接数据节点
  - 文件连接的数据节点为链表，分成多块在其他块储存
- 块
  - 0 ： super node, 存储整个文件系统的元数据
  - 1-64： inode, 存储某个目录/文件节点的各种元数据。
    - 每个块中存储多个inode
  - 64-max ： data_node, 存储数据块
