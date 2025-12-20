# gitlite设计思路

## 结构
### 项目骨架
```text
gitlite/
├── include/
│   ├── Utils.h
│   ├── GitliteException.h
│   ├── Repository.h                #执行gitlite命令
│   ├── Pointers.h                  #用于HEAD指针和分支指针相关操作
│   ├── Stage.h                     #用于staging area相关操作
│   ├── Commit.h                    #用于commit相关操作
│   └── Blob.h                      #用于blob相关操作
├── src/
│   ├── Utils.cpp
│   ├── GitliteException.cpp
│   ├── Repository.cpp
│   ├── Pointers.cpp
│   ├── Stage.cpp
│   ├── Commit.cpp
│   └── Blob.cpp
├── testing/
└── main.cpp
```
### .gitlite结构
```text
.gitlite/
├── HEAD                            # 文件，记录HEAD指针
├── branches/                       # 记录分支
│   ├── master                      # 文件，本地分支指针
│   ├── ...
│   ├── origin/                     # 文件夹
│   │   ├──master                   # 文件，本地仓库中的远程仓库分支(形如origin/master)指针
│   │   └── ...
│   └── ...
├── stage                           # 文件，记录暂存添加和暂存待删除
├── commits/
│   ├── 0c6924...(40位)             # commit文件，文件名为commit内容的SHA-1哈希值
│   └── ...
├── blobs/
│   ├── 972a1a...(40位)             # blob文件，文件名为相应文件内容的SHA-1哈希值
│   └── ...
└── remotes/
    ├── origin                      # 文件，记录远程仓库地址
    └── ...
```
## 文件记录方式
### HEAD
若HEAD与某分支指针相同，则文件内标记`ref: `，并记录分支指针地址
```text
ref: .gitlite/branches/[branchname]
```
若HEAD指针是detached HEAD状态，则文件内记录指向的`commit id`
### 分支指针
文件内为指向的`commit id`
### stage
对下文字符串序列化(Utils::serialize)存储
```text
a.txt 972a1a11f19934401291cc99117ec614933374ce      # 暂存待添加，文件名 blob文件名
-b.txt                                              # 暂存待删除，文件名前标记`-`
```
### commit文件
对下文字符串序列化存储，下文内容SHA-1哈希值记为`commit id`，文件名为`commit id`
```text
message: [message]
timestamp: [timestamp]                              # 记录从“Unix纪元”经过的时间
parent: [parent commit id]                          # 第一个父提交
parent: [parent commit id]                          # 如果是合并提交，记录第二个父提交
[filename] [blobhash]                               # 追踪的文件名及blob文件名
```
### blob文件
存储相应文件的序列化内容，文件名是对内容进行SHA-1得到的哈希值
### remotes下文件
文件名为远程仓库名称，内容为远程仓库地址
## 类的定义和工作原理
### Pointers
成员全部为静态成员函数，用于HEAD和branch相关操作，无需创建对象直接调用函数
### Stage
实例变量：一个字符串记录stage文件内容，两个map记录待添加和待删除文件

通过当前stage文件中内容创建Stage对象，通过Utils::readContentsAsString和从字符串中读取添加和删除文件反序列化
### Commit
实例变量：commit id, timestamp, parents, files

反序列化：readContentsAsString，并读取字符串中的提示词("message:"等)

通过public成员函数对变量进行获取和修改
### Blob
blob文件的创建和内容读取
### Repository
功能实现的核心，成员全部为静态成员函数，main函数通过调用Repository的public成员实现gitlite命令，private成员用于获取当前仓库信息（如当前提交、untracked files等）。
#### merge的实现
##### LCA查找
沿两个分支向前回溯，用map记录找到的祖先，用第二个值标记是哪个分支回溯到的。利用广度优先搜索，将待检查的父提交放入queue，这样保证每次取出的父提交到最初位置的距离是单调不降的。每次从queue中取出父提交，检查是否被另一侧追溯到过，如果没有，并且也没有被同侧追溯到过，就把其父提交全部加入queue，再次从queue中提取父提交，直到找到LCA。
##### 合并的7种情况和冲突检查
将当前提交和给定提交中的文件进行比对，分成4类用map记录：当前（给定）分支中发生修改(modify)，当前（给定）分支中与分割点相同(same)，存在于分割点但不存在于当前（给定）分支(not_in)，存在于当前（给定）分支但不存在于分割点(new_in)。

先检查未跟踪文件是否会被覆盖，防止遇到会覆盖的情况时无法撤销已经进行的修改。

1. 任何自分割点以来在给定分支中被修改过，但在当前分支中未被修改过的文件，即在same_in_current，且在modify_in_given，则checkout给定分支的版本并add暂存。
2. 自分割点以来，在当前分支中已修改但在给定分支中未修改的文件，即在modify_in_current，且在same_in_given，则不做处理。
3. 任何在当前分支和指定分支中以相同方式修改的文件在合并后保持不变。如果某个文件在当前分支和指定分支中都被删除，但工作目录中存在同名文件，则该文件将保持不变，并且在合并后仍处于既不被跟踪也不被暂存的状态。本情况无需执行任何操作。
4. 任何在分割点既不被跟踪也不被暂存且仅存在于当前分支中的文件，即在new_in_current，且不在new_in_given，则不做处理。
5. 任何在分割点既不被跟踪也不被暂存并且仅存在于给定分支中的文件，即在new_in_given，且不在new_in_current，则checkout给定分支的版本并add暂存。
6. 任何存在于分割点、在当前分支中未修改、在给定分支中既不被跟踪也不被暂存的文件，即在same_in_current，且在not_in_given，则执行rm删除。
7. 任何存在于分割点、在给定分支中未修改且在当前分支中既不被跟踪也不被暂存的文件，即在not_in_current，且在same_in_given，则不做处理。

对于冲突：

1. 两个均更改，即在modify_in_current，且在modify_in_given
2. 一个更改一个删除，即在modify_in_current且在not_in_given，或在not_in_current且在modify_in_given
3. 分割点不存在，两个分支都有且不同，即在new_in_current，且在mew_in_given，且不同

通过blob哈希值比较内容是否相同。在写冲突文件时，"\n"标记行结束，读出的文件结尾如果没有"\n"则写冲突文件时需要添加。
##### 合并提交
在commit函数中增加默认参数isMerge(flase)和mergeParent("")，通过这两个参数是否不同于默认值来判断是否是合并提交。
#### log的实现
对于某个分支的提交记录，从头提交开始，打印提交信息，然后用第一个父提交递归调用outputBranch函数。
#### 哈希值缩写
获取缩写的长度，从每个commit id中截取相同长度的前缀，比较是否相同
#### 远程仓库的处理
##### 查找要复制的commit
getFutureCommits函数。查找要达到的未来提交和历史提交之间的所有路径，用map记录。类似于深度优先搜索，从未来提交出发，每一步获取所有父提交，再从每个父提交出发寻找历史提交。一旦某条路径找到，函数不断向上层返回true，路径上所有commit都会被加入map中。当到达initial commit时，parents为空，返回false。

这里需要获取commit文件构造Commit对象，因此在Commit构造函数中增加一个默认参数(repoPath，默认为".gitlite")以传入远程仓库地址。
##### 形如origin/main的分支中`/`的处理
不将分支名作为文件名，直接将`/`解析为路径分隔符，远程分支名部分成为文件夹。这样存储时`/`并不影响路径拼接，对于`checkout [branchname]`等函数可以正常使用。对于HEAD指针指向分支的获取，通过截取`ref: .gitlite/branches/`后面的部分获取，因此没有影响。在set_ref函数中添加默认参数repoPath以方便修改远程HEAD指针。在获取所有分支名称时(用于status)，这些分支通过获取branches下所有文件夹名称(去除.和..)再和文件名拼接来获取。