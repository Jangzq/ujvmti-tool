#ujvmti-tool 
ujvmti-tool是一个用jvmti实现的工具集，用来从正在执行的java进程中得到诊断信息。


##Features

目前，提供了两个Heap工具。
+ 打印指定的类的引用。
+ 打印Root引用。

解决java内存问题，最有效的方法为：Dump堆信息，并且用内存分析工具，比如MAT进行分析。但是当进程的内存比较大时，dump堆和传输文件的开销就比较大了，本工具集中提供的工具并不能解决所有问题，但是可以快速的打印一些相关信息，可以从中发现一些痕迹。


##Install

1. 设置 JAVA_HOME 环境变量，指向JDK目录。
2. make
3. 拷贝 `libujvmti.so` 到被观测程序所在的主机。

##Usage
本工具的Agent在运行时加载，通过attach api动态载入，载入代码如下：
```java
            VirtualMachine virtualMachine = VirtualMachine.attach(pid);
            virtualMachine.loadAgentPath(agentPath, options);
            virtualMachine.detach();
```
+ pid: 目标java进程的pid。
+ agentPath: `libujvmti.so`的路径。
+ options: 区分具体操作，目前为以下两种。
`dump_refer` 打印指定的类的引用。
`dump_root` 打印Root引用。

###dump_refer
打印指定的类的引用。

**指定类配置文件**

文件格式：
类名,属性名,属性签名

类名：“JNI type signature of the class” 比如java.util.List写作"Ljava/util/List;"。

属性名和属性签名可以省略，如果设置，则打印这个对象的相应属性值。

**注意：目前只支持属性签名为I的情况，即返回值为整型的函数**

举例：

```java
Ljava/util/HashMap;,size,I
Ljava/util/ArrayList;,size,I
[Ljava/lang/String;

```

缺省“指定类配置文件”：
目标java进程的工作目录下的`ujvmfilter.cfg`。也可由调用virtualMachine.loadAgentPath时的options指定，格式为`dump_refer;文件路径`。

**输出文件**

目标java进程的工作目录下的`ujvmheapref.dat`，内容举例如下：

```
Reference Kind: JVMTI_HEAP_REFERENCE_STATIC_FIELD
Referrer: Lcom/sun/org/apache/xerces/internal/impl/Constants;  Referree: [Ljava/lang/String;
Array Length: 19
-----------------------------------------
Reference Kind: JVMTI_HEAP_REFERENCE_FIELD
Referrer: Lcom/sun/org/apache/xerces/internal/parsers/XIncludeAwareParserConfiguration;  Referree: Ljava/util/ArrayList;
Self Size: 24
size: 4
```

###dump_root
打印Root引用，包括"local variable on the stack", " JNI local reference", "JNI global reference"。
**输出文件**
目标java进程的工作目录下的`ujvmrootref.dat`，内容举例如下：

```
Reference Kind: JVMTI_HEAP_REFERENCE_STACK_LOCAL
Referree: [Ljava/lang/String;
Array Length: 1
Thread: main
Method: Lorg/apache/catalina/startup/Bootstrap;.main ([Ljava/lang/String;)V
-----------------------------------------
Reference Kind: JVMTI_HEAP_REFERENCE_STACK_LOCAL
Referree: Ljava/lang/String;
Self Size: 24
Thread: main
Method: Lorg/apache/catalina/startup/Bootstrap;.main ([Ljava/lang/String;)V
```

##Copyright and License
This software is licensed under the BSD license.

Copyright (C) 2015, by Zhang Zq (Jangzq)

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 



