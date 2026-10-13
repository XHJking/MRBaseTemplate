# MRBaseTemplate

基于 **Unreal Engine 5.6** 与 **PICO XR SDK** 的 PICO 4U 混合现实（MR / Video See-Through 透视）基础模板。

个人研究项目，用于学习与验证 MR 核心能力：VST 透视渲染、空间网格（Spatial Mesh）、空间锚点（Spatial Anchor）、场景语义捕获（Scene Capture）、大空间（LBE）多设备锚点共享，以及裸手手势戳触 3D UI 交互。

> 本模板的核心逻辑全部封装在 `Plugins/MRBasePlugin` 插件中，主工程模块只做最小化的壳，方便直接把插件拷走复用或以此为基础扩展自己的 MR 项目。

## 环境要求

| 依赖 | 版本 / 说明 |
|---|---|
| Unreal Engine | 5.6 |
| IDE | Visual Studio 2022（含 C++ 游戏开发工作负载） |
| 目标设备 | PICO 4U（需在系统设置中开启手部追踪、透视相关权限） |
| 打包平台 | Android（Min SDK 29，包名 `com.mr.baseclient`） |

## 项目结构

```
MRBaseTemplate/
├─ Config/                     # 引擎与项目配置（VST、空间网格/锚点开关等）
├─ Source/MRBaseTemplate/      # 主工程模块（最小化壳，仅注册模块）
├─ Plugins/
│  ├─ MRBasePlugin/            # ★ 自研核心插件（框架 + MR 能力封装 + 内容资产）
│  ├─ PICOXR/                  # PICO 官方 XR SDK 插件 v3.4.1（启用）
│  ├─ PICOOpenXR/              # PICO OpenXR 插件 v1.5.2（备用，当前禁用）
│  └─ PICOEnterprise/          # PICO 企业版插件 v3.3.2（启用，大空间能力依赖）
└─ SpatialMesh/                # 已导出的场地扫描数据 VenueScan.picomesh
```

## 插件说明

### MRBasePlugin（自研核心插件）

插件版本 1.0.0，模块类型 Runtime，含蓝图内容资产，依赖 PICOXR 插件。

#### 框架类

| 类 | 路径 | 说明 |
|---|---|---|
| `AMRGameMode` | `Pawn/… /GameMode/` | MR GameMode，默认启用全局配置中的 `GM_MRGameMode` 蓝图；支持 `MRPawnClassOverride` 覆盖默认 Pawn |
| `UMRGameInstance` | `GameInstance/` | 全局 GameInstance，处理应用恢复（Resume）等生命周期事件 |
| `AMRPawn` | `Pawn/` | MR 基础 Pawn：摄像机 + 左右 MotionController 组件，运行时由 PICOXR 驱动 6DoF 追踪与 XR 合成 |
| `AMRHandInteractionPawn` | `Pawn/` | 裸手近场戳触 Pawn：每帧从食指指尖沿指尖朝向做短距离球形扫描，命中 `UWidgetComponent` 即视为"戳到"，自动模拟鼠标按下、指尖离开自动抬起，Slider/按钮由此接管。可配置指尖扫描半径 `PokeSphereRadius`、判定距离 `PokeForwardDistance`、自测 Widget 距离 |

#### MR 能力封装组件（ActorComponent，全部暴露蓝图 API 与委托）

| 组件 | 说明 |
|---|---|
| `UAC_PC_MRSpatialAnchorComponent` | 空间锚点服务封装：启动/关闭锚点服务、创建/持久化/加载/销毁锚点、按 UUID 过滤加载、锚点与 Actor 类映射存入 SaveGame；提供 `OnAnchorCreated / OnAnchorPersisted / OnAnchorLoaded` 委托 |
| `UAC_PC_MRSpatialMeshComponent` | 空间网格扫描封装：启动/停止扫描、请求网格数据、LOD 设置（默认 High）、网格数据序列化保存到文件（如导出 `VenueScan.picomesh`）、绘制控制（开始/暂停/清除/显隐） |
| `UAC_PC_MRSceneCaptureComponent` | 场景语义捕获封装：启动捕获服务与应用、按语义标签过滤请求场景数据（Floor/Ceiling/Wall/Door/Window/Opening/Table 等）、查询 2D/3D 包围盒与多边形 |
| `UAC_PC_MRSptialAnchorSpace` | 大空间（LBE）地图共享模式锚点组件，见下文"大空间模式" |

#### 工具类与存档

| 类 | 说明 |
|---|---|
| `AStaticSpatialMeshActor` | 静态空间网格重建 Actor：从另一台头显序列化导出的扫描文件加载网格数据，用 `UProceduralMeshComponent` 分帧重建（`MeshesPerTick`），默认纯黑 Unlit 材质写深度实现**真实物体遮挡虚拟物体**（黑色像素被 VST 画面替换）；支持碰撞、语义顶点色、线框调试材质 |
| `USG_PICOMR` | SaveGame 对象：锚点 UUID → Actor 类映射表 + 大空间锚点空间数据（Transform + ActorClass） |

#### 内容资产

| 资产 | 说明 |
|---|---|
| `Maps/Lv_MR` | 白盒验证关卡（已配置为默认启动地图），运行时自动生成透视背景与白盒示例 |
| `GameMode/GM_MRGameMode`、`Pawn/BP_MRPawn` | 默认 GameMode / Pawn 蓝图 |
| `GameActor/BP_Sphere / BP_Box / BP_Projectile` | 白盒测试 Actor |
| `ActorComponent/AC_PC_*` | 各能力组件的蓝图封装 |
| `Python/create_mr_level.py` | 编辑器 Python 脚本，一键重建空白 Lv_MR 关卡 |

### PICO 官方 SDK 插件（第三方）

| 插件 | 版本 | 状态 | 作用 |
|---|---|---|---|
| PICOXR | 3.4.1 | 启用 | PICO XR 核心插件：HMD/手柄追踪、透视、空间锚点、空间网格、手势等能力底层 |
| PICOEnterprise | 3.3.2 | 启用 | 企业版插件：大空间地图共享等企业管理平台能力 |
| PICOOpenXR | 1.5.2 | 禁用 | PICO OpenXR 插件（备用方案，当前工程走 PICOXR 路线） |

## 核心功能特性

- **VST 透视渲染**：基于 PICOXR Video See-Through，配置前向渲染 + Instanced Stereo + Mobile MultiView，虚拟内容直接合成到真实画面上
- **空间网格**：启动扫描获取环境网格，支持 LOD、语义着色、导出文件与 `AStaticSpatialMeshActor` 离线重建（遮挡 + 碰撞）
- **空间锚点**：锚点创建/持久化/加载全流程封装，锚点与对应 Actor 类映射存入 SaveGame，重启后自动恢复场景
- **大空间（LBE）多设备锚点共享**：配合企业管理平台开启"大空间地图共享"后，所有设备共用同一系统地图、LocalFloor 追踪原点对齐同一物理空间，因此只需把锚点 `Transform + ActorClass` 存入 `.sav` 拷贝到其他设备，直接 `SpawnActor` 即出现在相同物理位置——**完全不依赖 PICO 云端与账号**
- **场景语义捕获**：识别地板/墙/门/窗/桌等语义结构，查询 2D/3D 包围盒与多边形
- **裸手戳触 3D UI**：食指指尖直接触碰 3D Widget，无手柄、无捏合，按钮与 Slider 可直接接管

## 快速开始

1. 克隆仓库，确认本机已安装 UE 5.6
2. 右键 `MRBaseTemplate.uproject` → **Generate Visual Studio project files**，用 VS 编译（或首次双击 uproject 时同意重新编译模块）
3. 编辑器内直接 PIE / VR 预览可验证关卡逻辑；真机验证流程：
   - 平台设置已预配 Android（包名 `com.mr.baseclient`），`Platforms → Android → Package Project` 出包
   - APK 安装到 PICO 4U，系统设置中开启**手部追踪**，并为应用授予透视/空间锚点等权限
4. 大空间模式：在 PICO 企业管理平台开启**大空间地图共享**，各设备扫描对齐后即可使用 `UAC_PC_MRSptialAnchorSpace` 的跨设备锚点能力；场地网格可经 `UAC_PC_MRSpatialMeshComponent::RequestAndSaveMeshInfos` 导出（参考根目录 `SpatialMesh/VenueScan.picomesh`），再由 `AStaticSpatialMeshActor::LoadAndBuild` 在任意设备上重建遮挡

## 渲染配置要点（Config/DefaultEngine.ini）

- 前向渲染 `r.ForwardShading=True`，关闭 Mobile HDR / 自动曝光 / 运动模糊（MR 标准配置）
- `vr.InstancedStereo` + `vr.MobileMultiView` 开启，MSAA 4x
- 虚拟阴影贴图（VSM）、Lumen GI、光线追踪已在移动 XR 允许范围内预配置
- PICOXR：`bEnableVST / bEnableAnchor / bEnableMesh=True`，`bEnableSceneCapture` 按需开启，Mesh LOD 默认 High

## 说明

- 本项目为个人研究用途，代码重点在于 MR 各子系统能力的封装与验证，不保证商用稳定性
- 若用于开源协作，建议配合 UE 官方 `.gitignore`（排除 `Binaries/ Intermediate/ Saved/ DerivedDataCache/ .vs/ .idea/ .svn/ Package/` 等目录）提交仓库
- PICO 官方插件（PICOXR / PICOOpenXR / PICOEnterprise）版权归 PICO / 字节跳动所有，仅作为运行依赖随工程分发

## License

仅作研究与学习用途，转载请注明出处。如需正式开源请自行选择并补充 LICENSE 文件。
