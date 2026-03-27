# Git 提交信息格式规范

## 🎯 技能定位

为 AI 助手和开发者提供 Git 提交信息的详细格式规范和操作指南，确保所有提交符合 Conventional Commits 标准。

## 适用范围

本规范适用于：

**AI 助手：**

- 生成规范的 Git 提交信息
- 验证提交信息质量
- 提供场景化建议

**开发者：**

- 日常开发提交
- Bug 修复提交
- 文档更新提交
- 配置变更提交
- 代码重构提交

---

## 📝 基本格式

### 标准结构

```text
<类型>: <简短描述>

[可选的详细说明]

[可选的脚注/引用]
```

### 格式要素

1. **标题行** - 必填，单行摘要
2. **空行** - 分隔标题和正文
3. **正文** - 可选，详细描述
4. **空行** - 分隔正文和脚注
5. **脚注** - 可选，引用 Issue/PR

---

## 🏷️ 提交类型（Type）

### 核心类型（必须掌握）

| 类型 | 含义 | 使用场景 | 示例 |
|------|------|---------|------|
| `feat` | 新功能 | 添加之前没有的功能 | `feat: 添加 SPI 通信功能` |
| `fix` | 修复 Bug | 修正错误的行为 | `fix: 修复内存泄漏问题` |
| `docs` | 文档 | 只修改文档/注释 | `docs: 更新函数参数说明` |
| `refactor` | 重构 | 改代码结构但不改功能 | `refactor: 提取公共函数` |
| `chore` | 杂项 | 配置、工具等不改动代码 | `chore: 更新.gitignore` |

**记忆技巧：** 这 5 个类型占日常提交的 95% 以上，优先掌握！

### 进阶类型（按需使用）

| 类型 | 含义 | 示例 |
|------|------|------|
| `perf` | 性能优化 | `perf: 优化 UART 接收缓冲区大小` |
| `style` | 代码格式 | `style: 格式化 main.c 缩进` |
| `test` | 测试相关 | `test: 添加 LED 控制单元测试` |
| `build` | 构建系统 | `build: 添加依赖库` |
| `ci` | CI/CD | `ci: 更新 GitHub Actions 配置` |

### 项目专用类型

针对嵌入式开发项目的特殊需求：

| 类型 | 含义 | 使用场景 | 示例 |
|------|------|---------|------|
| `sync` | CubeMX 同步 | 与 STM32CubeMX 同步代码 | `sync: 同步 CubeMX 配置` |
| `hotfix` | 紧急修复 | 生产环境紧急补丁 | `hotfix: 修复串口通信崩溃` |

**注意：** `sync` 是本项目特有类型，用于标注 CubeMX 同步操作。

---

## 📏 长度限制

### 标题行

- **英文：** ≤50 个字符（包括类型和前缀）
- **中文：** ≤25 个汉字（包括类型和前缀）
- **格式：** `<类型>: <描述>`（冒号后必须有空格）

✅ **正确示例：**

```text
feat: 添加串口命令解析功能
fix: 修复定时器中断响应延迟
chore: 更新 VSCode 配置
```

❌ **错误示例：**

```text
feat: 今天花了一下午终于把这个该死的 bug 修好了主要是定时器中断的问题还有 GPIO 初始化顺序也不对导致系统经常死机
```

### 正文

- **每行长度：** ≤72 个字符
- **列表项：** 每项独立成行
- **段落间距：** 空行分隔

### 脚注

- **Issue 引用：** `Closes #12`、`Fixes #34`
- **PR 引用：** `Merge pull request #56`
- **多引用：** 每行一个引用

---

## ✍️ 语义要求

### 祈使句语气

使用现在时态，避免过去式：

✅ **推荐：**

```text
feat: 添加 LED 闪烁控制
fix: 修复串口打印乱码
refactor: 提取 GPIO 初始化函数
```

❌ **避免：**

```text
feat: 添加了 LED 闪烁控制
fix: 修复了串口打印乱码
refactor: 提取了 GPIO 初始化函数
```

### 清晰具体

避免模糊描述，明确指出变更内容：

✅ **推荐：**

```text
feat: 添加 PWM 占空比动态调整功能
fix: 修正 I2C 总线时序错误
docs: 更新 README 硬件连接说明
```

❌ **避免：**

```text
update
修改了一些代码
继续开发
小修小补
```

### 首字母大小写

- **英文：** 标题首字母小写（专有名词除外）
- **中文：** 无特殊要求
- **末尾：** 不加标点符号

✅ **正确：**

```text
feat: add LED blink control
feat: 添加 LED 闪烁控制
```

❌ **错误：**

```text
feat: Add LED blink control
feat: 添加 LED 闪烁控制。
```

---

## 📋 正文格式

### 何时需要正文

满足以下条件时应添加正文：

1. 变更超过 3 处独立修改
2. 需要解释"为什么"而不是"做了什么"
3. 有重要的背景信息或技术决策
4. 包含破坏性变更或迁移指南

### 正文结构

```text
<类型>: <标题>

第一段：概述变更背景和目的

第二段：详细说明关键技术点（可选）

- 关键修改点 1
- 关键修改点 2
- 关键修改点 3

第三段：其他补充信息（可选）
```

### 列表示例

✅ **推荐：**

```text
feat: 添加多 LED 支持

- 新增 led_init_all() 批量初始化函数
- 支持最多 8 个 LED 控制
- 更新 LED_PIN 宏定义
- 添加 LED 状态管理机制
```

❌ **避免：**

```text
feat: 添加多 LED 支持
- 新增 led_init_all() 批量初始化函数
- 支持最多 8 个 LED 控制
- 更新 LED_PIN 宏定义
- 添加 LED 状态管理机制
```

---

## 🔗 脚注格式

### Issue 引用

```text
Closes #12
Fixes #34
Resolves #56
Refs #78
```

### 多 Issue 引用

```text
Closes #12
Closes #34
Refs #56, #78
```

### PR 引用

```text
Merge pull request #12 from user/feature-branch

Description of the change
```

---

## 💡 最佳实践

### 嵌入式开发场景

#### CubeMX 同步

```text
# 首次导入
feat: 从 CubeMX 导入初始工程

# 常规同步
sync: 同步 CubeMX 配置 (启用 SPI2)

# 大量修改
sync: CubeMX 代码同步 - 更新 USART 和定时器配置
```

#### HAL 库开发

```text
# 新驱动
feat: 添加 OLED 显示屏驱动 (SSD1306)

# 修复问题
fix: 修复 I2C 读写时序问题

# 调试相关
feat: 添加调试用 LED 闪烁模式
```

#### 配置文件

```text
# PlatformIO
chore: 添加 release 编译环境

# VSCode
chore: 添加代码片段模板

# 脚本
chore: 更新 sync.bat 脚本
```

### 决策树（快速选择提交类型）

```text
这次提交做了什么？
│
├─ 加了新功能？ → feat
│   └─ 例：feat: 添加 SPI 通信功能
│
├─ 修了 Bug？ → fix  
│   └─ 例：fix: 修复内存泄漏问题
│
├─ 只改了文档/注释？ → docs
│   └─ 例：docs: 更新函数参数说明
│
├─ 改了代码结构但功能不变？ → refactor
│   └─ 例：refactor: 提取公共函数到 utils.c
│
├─ 优化了性能？ → perf
│   └─ 例：perf: 优化 UART 接收缓冲区大小
│
├─ 只是格式化代码？ → style
│   └─ 例：style: 格式化 main.c 缩进
│
├─ 测试相关？ → test
│   └─ 例：test: 添加 LED 控制单元测试
│
└─ 其他（配置、工具等）？ → chore
    └─ 例：chore: 更新.gitignore
```

**使用提示：** 从上到下依次判断，第一个匹配的就是最合适的类型。

---

## ✅ 质量检查清单

在提交前逐项检查：

- [ ] **格式完整** - 使用 `<类型>: <描述>` 格式
- [ ] **类型准确** - 选择了最合适的提交类型
- [ ] **长度合规** - 标题≤50 字符，正文每行≤72 字符
- [ ] **语义明确** - 使用祈使句，避免模糊描述
- [ ] **拼写检查** - 无拼写和语法错误
- [ ] **标点正确** - 标题末尾无标点，冒号后有空格
- [ ] **正文必要** - 复杂变更已添加详细说明
- [ ] **脚注规范** - Issue/PR 引用格式正确（如适用）
- [ ] **一次一事** - 本次提交只做一件事
- [ ] **及时提交** - 完成一个小功能就提交

---

## 🔧 自动化工具

### VSCode 集成

在 `.vscode/settings.json` 中配置：

```json
{
  "git.enableSmartCommit": true,
  "git.smartCommitChanges": "tracked",
  "git.postCommitCommand": "pull"
}
```

### PowerShell 快捷命令

在 `$PROFILE` 中添加：

```powershell
# Git 快速提交
function gitcommit {
    param(
        [Parameter(Mandatory=$true)]
        [string]$msg
    )
    git add .
    git commit -m $msg
    Write-Host "✓ 提交成功：$msg" -ForegroundColor Green
}

# 使用示例
gitcommit "feat: 添加串口命令解析"
```

### Git 别名

在 `.gitconfig` 中添加：

```ini
[alias]
    c = commit
    ca = commit -a
    cm = commit -m
    amend = commit --amend
```

---

## 📊 实际案例

### 项目启动阶段

```text
feat: 创建 STM32F103C8 基础工程
feat: 实现 LED 闪烁功能
feat: 配置串口通信 (115200bps)
docs: 完善 README.md 使用说明
```

### 开发阶段

```text
fix: 修复串口打印乱码 (波特率不匹配)
feat: 添加命令行解析功能
refactor: 重构 LED 控制接口
perf: 优化延时函数精度
docs: 添加 DAP-Link 调试指南
```

### 稳定维护阶段

```text
fix: 边界条件检查 (#23)
docs: 更新 API 文档
chore: 清理调试代码
test: 添加基础功能测试
```

---

## ⚠️ 常见错误

### 太模糊

❌ **错误：**

```text
update
修改了一些代码
继续开发
```

✅ **正确：**

```text
feat: 添加 PWM 输出功能
fix: 修正串口乱码问题
chore: 删除调试代码
```

### 太长

❌ **错误：**

```text
feat: 这是一个非常非常长的提交信息描述了所有修改细节包括为什么要这样改以及改了什么地方还有未来计划
```

✅ **正确：**

```text
feat: 添加 PWM 占空比动态调整功能

- 新增 pwm_set_duty() 函数
- 支持 0-100% 范围设置
```

### 类型误用

❌ **错误：**

```text
feat: 更新 README 安装说明
```

✅ **正确：**

```text
docs: 更新 README 安装说明
```

---

## 📚 参考资源

- **Conventional Commits** - <https://www.conventionalcommits.org/>
- **Git Commit Message Guidelines** - <https://cbea.ms/git-commit/>
- **Angular Commit Guidelines** - <https://github.com/angular/angular/blob/main/CONTRIBUTING.md#commit>

---

**版本：** v1.0  
**最后更新：** 2026-03-17
