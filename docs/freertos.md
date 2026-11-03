# FreeRTOS 接入

`ports/freertos/cabinet_task.c` 提供控制任务、任务队列、ISR 消息投递和状态快照。它需要调用方提供实际 FreeRTOS 内核、架构端口、`FreeRTOSConfig.h` 和硬件接口，不会被默认的主机构建链接。

## 所有权

```text
串口 ISR -- xQueueSendFromISR --> 控制任务 --> controller
传感器任务 -- xQueueSend ------> 控制任务 --> controller
读取任务 -- mutex ------------> published snapshot
```

控制任务是核心对象的唯一运行时所有者。队列项包含串口字节副本或业务事件，投递后生产者可复用原缓冲。ISR 接口一次接收最多 64 字节；超长块必须在驱动侧拆分。事件或串口队列溢出设置无锁原子标志，由控制任务消费后进入停止锁存。

串口 ISR 的优先级必须符合端口对 FreeRTOS `FromISR` API 的限制。应在 `cabinet_task_start()` 成功后再开放生产者中断；该启动接口只允许由一个初始化上下文调用。当前要求 `unsigned` 原子操作无锁，不能满足此条件的平台需要另外设计临界区。

## 启动顺序

1. 初始化硬件并保持功率输出关闭，准备独立硬件保护。
2. 创建并填充 `cab_ports_t`。串口发送提交非阻塞发送，开锁和功率调用应快速返回。
3. 调用 `cabinet_task_start(&ports)` 并检查返回值。
4. 启动调度器，开放外设生产者；传感器经 `cabinet_task_post()` 上报状态。
5. 串口 ISR 只调用 `cabinet_task_receive_from_isr()`，不能直接访问核心对象。
6. 其他任务通过 `cabinet_task_snapshot()` 读取受互斥保护的快照。

任务默认优先级为 `tskIDLE_PRIORITY + 2`、栈深度 1024 个 `StackType_t` 单元。适配层使用动态创建 API，仅在启动时分配资源；业务核心没有堆分配。资源值需在目标板上测量、调整，不能把这里的栈深度当作已验证的硬件结论。

控制任务空闲时最多等待约 10 ms，收到消息后处理有限数量并运行一次控制步骤。每次最多消费 32 条队列消息，避免持续输入让状态机完全失去运行机会。Tick 转为毫秒时保留余数，并通过 Tick 差值处理回绕。不能暂停任务一个完整 Tick 回绕周期。

快照采用短时互斥访问，控制任务不等待读者持锁。没有读到新快照时读者可能得到上一次状态，应检查 `steps` 是否变化。硬件看门狗的喂狗策略、任务失活检测与紧急停机应由系统级监控另行实现。

## 配置与升级入口

当前任务消息公开业务事件和串口输入，没有暴露跨任务的配置或升级命令。板级工程如需调用这些核心接口，应增加带容量限制的命令消息并由控制任务执行，不能从网络任务直接访问内部 `controller`。主机场景已串行调用并验证相关核心流程。

## 构建检查

已使用真实的 [FreeRTOS-Kernel V11.2.0](https://github.com/FreeRTOS/FreeRTOS-Kernel/releases/tag/V11.2.0) 头文件和官方 MSVC-MingW 端口头文件执行语法/API 兼容性检查。内核不随本仓库分发。测试配置放在 `tests/freertos/`，仅供头文件检查，不作为板级配置。

```bash
python tools/check_freertos.py --kernel /path/to/FreeRTOS-Kernel \
    --port /path/to/FreeRTOS-Kernel/portable/MSVC-MingW
```

目标板应提供自己的配置目录及交叉编译器：

```bash
python tools/check_freertos.py --kernel /path/to/FreeRTOS-Kernel \
    --port /path/to/kernel/portable/GCC/ARM_CM4F \
    --config /path/to/board/config --cc arm-none-eabi-gcc
```

若以 CMake 子目录接入，先创建并配置 `freertos_kernel` 目标，再启用 `CABINET_FREERTOS_ADAPTER`。编译调用点与头文件兼容不等同于调度器运行验证；本仓库没有已验证的具体 MCU 固件或烧录产物。
