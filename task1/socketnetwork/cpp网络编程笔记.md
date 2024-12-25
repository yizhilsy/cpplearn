## C/S架构的客户端-服务端

### 一个完整的TCP连接中，服务端和客户端通信需要做三件事：

* 服务端与客户端进行连接
* 服务端与客户端之间传输数据
* 服务端与客户端之间断开连接

### 在[socket编程](https://zhida.zhihu.com/search?content_id=114593860&content_type=Article&match_order=1&q=socket%E7%BC%96%E7%A8%8B&zhida_source=entity)中，服务端和客户端是靠**socket**进行连接的。服务端在建立连接之前需要做的有：

* 创建socket（伪代码中简称为 `socket()`）
* 将[socket](https://zhida.zhihu.com/search?content_id=114593860&content_type=Article&match_order=8&q=socket&zhida_source=entity)与指定的IP和端口（以下简称为port）绑定（伪代码中简称为 `bind()`）
* 让socket在绑定的端口处监听请求（服务端监听客户端将要连接到服务端绑定的端口）（伪代码中简称为 `listen()`）

### 客户端发送连接请求并成功连接之后（这个步骤在伪代码中简称为 `accept()`），服务端便会得到**客户端的套接字**，于是所有的收发数据便可以在这个客户端的套接字上进行了。

而收发数据其实就是：

* 接收数据：使用客户端套接字拿到客户端发来的数据，并将其存于buff中。（伪代码中简称为 `recv()`）
* 发送数据：使用客户端套接字，将buff中的数据发回去。（伪代码中简称为 `send()`）

### 在收发数据之后，就需要断开与客户端之间的连接。在socket编程中，只需要关闭客户端的套接字即可断开连接。（伪代码中简称为 `close()`）

### 客户端相对于服务端来说会简单一些。它需要做的事情有：

* 创建socket
* 使用socket和已知的服务端的ip和port连接服务端
* 收发数据
* 关闭连接

### select的基本流程

* 初始化fd_set集合，将需要监视的文件描述符加入到相应的集合中。
* 调用select函数，等待文件描述符就绪或超时。
* 检查select返回值，处理就绪的文件描述符。
* 根据需要重复以上步骤。
