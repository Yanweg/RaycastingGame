#include <cmath>
#include <string>
#include <vector>
#include <iostream>
#include <windows.h>

#include <easyx.h>
#include <graphics.h>



#define screenWidth  640			// 屏幕宽度（每一列对应一条射线）
#define screenHeight 480		// 屏幕高度
#define mapWidth 24
#define mapHeight 24

// 鼠标控制相关变量
int lastMouseX = 320;          // 上一帧鼠标X坐标（窗口中心640/2=320）
double mouseSensitivity = 0.002; // 鼠标灵敏度（数值越大转得越快）

// ===================== 游戏世界地图（核心数据）=====================
// 2D网格地图：0=空地，1~5=不同颜色的墙体
// 所有3D视觉效果，都基于这个纯2D地图计算而来
int worldMap[mapWidth][mapHeight] =
{
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,2,2,0,2,2,0,0,0,0,3,0,3,0,3,0,0,0,1},
  {1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,3,0,0,0,1},
  {1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,2,2,0,2,2,0,0,0,0,3,0,3,0,3,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,4,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,4,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,0,0,0,4,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,4,0,4,0,0,0,0,4,0,0,0,0,0,0,0,3,0,0,0,0,0,0,1},
  {1,4,0,4,4,4,4,4,4,0,0,0,0,0,0,0,3,0,0,0,0,0,0,1},
  {1,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,1},
  {1,4,4,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};


// ===================== 主函数（程序入口）=====================
int main(int argc, char* argv[])
{
    // ===================== 玩家状态初始化 =====================
    double posX = 22, posY = 12;    // 玩家在2D地图中的坐标 (x, y)
    double dirX = -1, dirY = 0;     // 玩家朝向向量（初始面朝左）
    double planeX = 0, planeY = 0.66;// 相机平面向量（与朝向垂直）
    // planeY=0.66 → 视野FOV≈66°（人眼舒适视角）
    double time = 0;                // 当前帧时间
    double oldTime = 0;             // 上一帧时间（计算帧率/移动速度）

    // 初始化绘图窗口：宽度、高度、位深、窗口标题
    initgraph(screenWidth, screenHeight);
    BeginBatchDraw();
   
    
    // ===================== 游戏主循环（永不结束，直到关闭窗口）=====================
    while (1) // done()：检测窗口是否关闭/退出
    {   
        // ===================== 核心：遍历屏幕每一列，发射一条射线 =====================
        // 屏幕宽度640 → 一帧仅计算640条射线，性能极高
        for (int x = 0; x < screenWidth; x++)
        {
            // 1. 计算相机X坐标：将屏幕x列映射到 [-1, 1] 区间（左=-1，中=0，右=1）
            double cameraX = 2 * x / (double)screenWidth - 1;
            // 2. 计算当前射线的方向向量（结合玩家朝向+相机平面）
            double rayDirX = dirX + planeX * cameraX;
            double rayDirY = dirY + planeY * cameraX;

            // 3. 射线当前所在的地图网格坐标（整数，对应worldMap的下标）
            int mapX = (int)posX;
            int mapY = (int)posY;

            // ===================== DDA算法初始化（光线投射核心算法）=====================
            // 射线在x/y方向上，每前进1个网格需要的距离（无符号，固定值）
            double deltaDistX = std::abs(1 / rayDirX);
            double deltaDistY = std::abs(1 / rayDirY);

            double perpWallDist; // 墙体到相机平面的垂直距离（用于修正鱼眼效果）
            int stepX, stepY;    // 射线的步向：x=-1向左，x=1向右；y=-1向上，y=1向下
            double sideDistX;    // 射线到下一个x网格边的距离
            double sideDistY;    // 射线到下一个y网格边的距离

            int hit = 0;  // 碰撞标记：0=未撞墙，1=已撞墙
            int side = 0; // 墙面标记：0=撞东西向墙（垂直面），1=撞南北向墙

            // 4. 设置射线的步向 + 初始侧距
            if (rayDirX < 0)
            {
                stepX = -1;
                sideDistX = (posX - mapX) * deltaDistX;
            }
            else
            {
                stepX = 1;
                sideDistX = (mapX + 1.0 - posX) * deltaDistX;
            }
            if (rayDirY < 0)
            {
                stepY = -1;
                sideDistY = (posY - mapY) * deltaDistY;
            }
            else
            {
                stepY = 1;
                sideDistY = (mapY + 1.0 - posY) * deltaDistY;
            }

            // ===================== DDA核心循环：沿射线前进，直到撞墙 =====================
            // 纯加减法计算，无复杂运算，CPU效率极高
            while (hit == 0)
            {
                // 向距离更近的网格边移动（x或y方向）
                if (sideDistX < sideDistY)
                {
                    sideDistX += deltaDistX; // 累加x方向步长
                    mapX += stepX;           // 更新网格x坐标
                    side = 0;                // 标记为x方向墙面
                }
                else
                {
                    sideDistY += deltaDistY; // 累加y方向步长
                    mapY += stepY;           // 更新网格y坐标
                    side = 1;                // 标记为y方向墙面
                }

                // 判断是否撞到墙体（地图值>0即为墙）
                if (worldMap[mapX][mapY] > 0) hit = 1;
            }

            // ===================== 计算墙体距离（关键：修正鱼眼效应）=====================
            // 不用直线距离！用「垂直相机平面的距离」，避免画面边缘拉伸变形
            if (side == 0)
                perpWallDist = (sideDistX - deltaDistX);
            else
                perpWallDist = (sideDistY - deltaDistY);

            // ===================== 计算墙体绘制高度 =====================
            // 距离越远，墙体越矮；距离越近，墙体越高
            int lineHeight = (int)(screenHeight / perpWallDist);

            // 计算墙体在屏幕上的绘制起止坐标（垂直居中）
            int drawStart = -lineHeight / 2 + screenHeight / 2;
            if (drawStart < 0) drawStart = 0; // 防止超出屏幕顶部
            int drawEnd = lineHeight / 2 + screenHeight / 2;
            if (drawEnd >= screenHeight) drawEnd = screenHeight - 1; // 防止超出屏幕底部

            // ===================== 设置墙体颜色 =====================
            // 根据地图中的墙体编号，分配不同颜色
            COLORREF color;
            switch (worldMap[mapX][mapY])
            {
            case 1: color = RED;   break; // 红色墙
            case 2: color =GREEN; break; // 绿色墙
            case 3: color =BLUE;  break; // 蓝色墙
            case 4: color =WHITE; break; // 白色墙
            default: color =YELLOW; break;// 黄色墙（默认）
            }

            // 侧面墙体亮度减半（视觉增强立体感，区分正反面）
            if (side == 1) color = color / 2;

            // ===================== 绘制墙体：每列只画1条竖线 =====================
            // 竖线拼接 → 形成3D透视的墙面效果
            setlinecolor(color);
           
            line(x, drawStart, x, drawEnd);
            
        }




        // ===================== 帧率计算 + 移动速度同步 =====================
        oldTime = time;
        time = GetTickCount64();
        double frameTime = (time - oldTime) / 1000.0; // 帧耗时（秒）

        // ===================== 鼠标控制视角旋转（核心）=====================
        POINT mousePos;
        GetCursorPos(&mousePos); // 获取鼠标屏幕坐标
        ScreenToClient(GetHWnd(), &mousePos); // 转为窗口内坐标

        // 计算鼠标水平移动距离
        int dx = mousePos.x - lastMouseX;
        lastMouseX = mousePos.x;

        // 鼠标旋转：dx>0右移=右转，dx<0左移=左转
        if (dx != 0)
        {
            double angle = -dx * mouseSensitivity; // 旋转角度,dx:+or-反转鼠标
            // 旋转方向向量
            double oldDirX = dirX;
            dirX = dirX * cos(angle) - dirY * sin(angle);
            dirY = oldDirX * sin(angle) + dirY * cos(angle);
            // 旋转相机平面（保持垂直）
            double oldPlaneX = planeX;
            planeX = planeX * cos(angle) - planeY * sin(angle);
            planeY = oldPlaneX * sin(angle) + planeY * cos(angle);
        }

        RECT rc;
        GetClientRect(GetHWnd(), &rc); // 获取绘图区
        ClientToScreen(GetHWnd(), (LPPOINT)&rc); // 转屏幕坐标
        SetCursorPos(rc.left + 320, rc.top + 9999); // 居中, +9999隐藏光标
        lastMouseX = 320;
        
        
        
        // ===================== 键盘控制：玩家移动/视角旋转 =====================
        double moveSpeed = frameTime * 5.0; // 移动速度（与帧率绑定，保证速度稳定）
        double rotSpeed = frameTime * 3.0;  // 旋转速度

        // 方向键↑：向前移动
        if ((GetAsyncKeyState(0x57) & 0x8000) || (GetAsyncKeyState(VK_UP) & 0x8000))
        {
            // 碰撞检测：前方是空地才移动
            if (worldMap[(int)(posX + dirX * moveSpeed)][(int)posY] == 0)
                posX += dirX * moveSpeed;
            if (worldMap[(int)posX][(int)(posY + dirY * moveSpeed)] == 0)
                posY += dirY * moveSpeed;
        }
        // 方向键↓：向后移动
        if ((GetAsyncKeyState(0x53) & 0x8000) || (GetAsyncKeyState(VK_DOWN) & 0x8000))
        {
            if (worldMap[(int)(posX - dirX * moveSpeed)][(int)posY] == 0)
                posX -= dirX * moveSpeed;
            if (worldMap[(int)posX][(int)(posY - dirY * moveSpeed)] == 0)
                posY -= dirY * moveSpeed;
        }
        // A / ← ：左平移（侧移）
        if ((GetAsyncKeyState(0x41) & 0x8000) || (GetAsyncKeyState(VK_LEFT) & 0x8000))
        {
            // 左移方向：垂直于朝向，左侧
            double leftX = -dirY;
            double leftY = dirX;
            if (worldMap[int(posX + leftX * moveSpeed)][int(posY)] == 0)
                posX += leftX * moveSpeed;
            if (worldMap[int(posX)][int(posY + leftY * moveSpeed)] == 0)
                posY += leftY * moveSpeed;
        }
        // D / → ：右平移（侧移）
        if ((GetAsyncKeyState(0x44) & 0x8000) || (GetAsyncKeyState(VK_RIGHT) & 0x8000))
        {
            // 右移方向：垂直于朝向，右侧
            double rightX = dirY;
            double rightY = -dirX;
            if (worldMap[int(posX + rightX * moveSpeed)][int(posY)] == 0)
                posX += rightX * moveSpeed;
            if (worldMap[int(posX)][int(posY + rightY * moveSpeed)] == 0)
                posY += rightY * moveSpeed;
        }

        //ESC退出
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
        {
            closegraph();  // 关闭图形窗口
            return 0;      // 退出程序
        }
        
        // 刷新屏幕，渲染当前帧画面
        FlushBatchDraw();
        // 清空屏幕背景（黑色），为下一帧绘制做准备
        cleardevice();
    }

    // 程序正常退出
    
    return 0;
}

