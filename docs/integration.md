# 接口使用

## 主机控制循环

```c
host_port_t host;
cab_controller_t controller;
cab_ports_t ports;

host_port_init(&host);
ports = host_port_bind(&host);
if (cab_controller_init(&controller, &ports) != CAB_OK)
    return 1;

host_observe(&host, &controller, 0u, true, true);
host_pump(&host, &controller, 0u);
host_pump(&host, &controller, 10u);
```

完整流程见 `apps/simulator/main.c`。主机时间由场景传入，不调用真实睡眠，因此整条 8 秒场景可以快速重复运行。

## 硬件接口合同

| 接口 | 调用方需保证 |
| --- | --- |
| `serial_send` | 调用结束前复制数据或启动可靠发送；不重入接收函数；返回失败表示未能提交 |
| `unlock` | 非阻塞提交开锁动作；返回值表示命令受理，不代表门已经打开 |
| `set_power` | 单仓功率设定；接受零功率；反馈异常返回失败；真实输出另行测量确认 |
| `nv_read` | 完整读取指定逻辑副本，禁止越界或部分成功 |
| `nv_write` | 擦写一个逻辑副本，不破坏另一个副本；每次覆盖 48 字节 |
| `image_write` | 按偏移写入暂存区，写入范围由模块检查，介质擦除由 HAL 负责 |
| `image_read` | 从同一暂存区读回，供最终完整性检查 |

读写失败应返回 `false`。不要在异步写尚未完成时返回成功，除非适配层已经保证后续读回能观察到完整内容。当前没有实际 Flash 地址、芯片寄存器或板级引脚定义。

观察事件使用整个门状态与在位状态快照。传感器需要在驱动或采样任务完成去抖和输入有效性检查，再按发生顺序入队。核心不负责传感器电气诊断。

## 换电请求

```c
cab_event_t request = {CAB_EVENT_EXCHANGE, 0u, 1u, false, false};
bool queued = cab_controller_post(&controller, &request);
```

`queued` 只表示成功入队。控制步骤消费事件后，`last_command` 表示最近消费事件的处理结果，后续事件可能覆盖它。此字段不是多请求应答通道，业务服务需要增加请求标识与完成通知。

当前需要调用者指定仓位，使用 0 开始的仓位索引；线路从站地址是 1 开始，两者不能混用。一次只允许一个换电流程。

## 配置和升级

配置通过 `cab_controller_configure()` 先持久化，再更新内存。换电进行中或升级接收中拒绝变更。参数成功更新后先请求全部零功率，再由下一控制周期按新预算恢复。

固件接收流程：

```text
update_begin(size, expected_crc)
    -> update_chunk(offset=0, ...)
    -> update_chunk(offset=previous_received, ...)
    -> update_finish()
    -> READY / FAILED
```

重复、乱序、空块或越界块会被拒绝；合法调用可以继续从当前已接收偏移发送。I/O 失败、CRC 错误与接收超时将进入失败状态。重新开始会重新初始化接收状态。

接收期间暂停所有仓位充电，并拒绝新换电。暂存校验完成或失败后，正常仓位可以在下一周期恢复充电；该模块不会切换执行中的程序。CRC32 不具有抗伪造能力，真实远程升级必须另行实现可信签名、版本限制、掉电处理和 Bootloader 策略。

## 日志与诊断

核心没有文本调试打印。故障通过固定容量 `cab_journal_t` 保存时间、类型、仓位和原因，`cab_journal_get()` 按最新到最旧读取。满容量后覆盖旧记录；如需长期追踪，应在上层建立可靠的持久化通道。

仿真程序使用 `fwrite` 输出 JSON 轨迹文件。这属于场景结果导出，默认没有终端业务日志，也没有实际设备标识或云平台凭据。
