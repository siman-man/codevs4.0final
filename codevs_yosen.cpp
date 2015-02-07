#include <iostream>
#include <fstream>
#include <vector>
#include <cassert>
#include <map>
#include <algorithm>
#include <limits.h>
#include <string>
#include <string.h>
#include <sstream>
#include <set>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <stack>
#include <queue>

using namespace std;

typedef long long ll;

// 役割一覧
const int WORKER          =  0; // ワーカー
const int KNIGHT          =  1; // ナイト
const int FIGHTER         =  2; // ファイター
const int ASSASIN         =  3; // アサシン
const int CASTEL          =  4; // 城
const int VILLAGE         =  5; // 村
const int BASE            =  6; // 拠点
const int COMBATANT       =  7; // 戦闘員
const int LEADER          =  8; // 戦闘隊長
const int COLLIERY        =  9; // 炭鉱(資源マスにワーカーが5人いる状態)
const int RESOURCE_BREAK  = 10; // 相手の村をひたすら破壊する族
const int KING            = 11; // 王様
const int GUARDIAN        = 12; // 守護者
const int GHQ             = 13; // 作戦司令本部(自軍の城の上にある拠点)

// 行動の基本優先順位
int movePriority[14] = { 5, 9, 8, 7, 0, 10, 15, 1, 20, 0, 0, 30, 50, 99};

// 行動一覧
const int NO_MOVE         =  0; // 何も移動しない
const int MOVE_UP         =  1; // 上に移動
const int MOVE_DOWN       =  2; // 下に移動
const int MOVE_LEFT       =  3; // 左に移動
const int MOVE_RIGHT      =  4; // 右に移動
const int CREATE_WORKER   =  5; // ワーカーを生産
const int CREATE_KNIGHT   =  6; // ナイトを生産
const int CREATE_FIGHTER  =  7; // ファイターを生産
const int CREATE_ASSASIN  =  8; // アサシンを生産
const int CREATE_CASTEL   =  9; // 城を生産
const int CREATE_VILLAGE  = 10; // 村を生産
const int CREATE_BASE     = 11; // 拠点を生産

// 試合状況一覧
const int OPENING = 0;  // 序盤戦
const int WARNING = 1;  // 敵ユニットを検知
const int DANGER  = 2;  // 自軍の城の視野に敵を確認
const int ONRUSH  = 3;  // 突撃(敵の城が見つかり倒しに行く状態)

// 敵一覧
const int CHOKUDAI  = 0;
const int COLUN     = 1;
const int GELB      = 2;
const int GRUN      = 3;
const int LILA      = 4;
const int ROSA      = 5;
const int SCHWARZ   = 6;
const int ZINNOVER  = 7;
const int SILVER    = 8;

// ユニットの行動タイプ
const int NONE            = 0; // 何もしない(何も出来ない)
const int SEARCH          = 1; // 探索(空いてないマスを探索)
const int DESTROY         = 2; // 破壊(敵を見つけて破壊)
const int PICKING         = 3; // 資源採取
const int STAY            = 4; // 待機命令
const int RESOURCE_BREAKE = 5; // 相手の資源マスを狙う
const int SPY             = 6; // 敵に見つからないように行動

// 各種最大値
const int OPERATION_MAX = 12;   // 行動の種類
const int UNIT_MAX = 7;         // ユニットの種類
const int COST_MAX = 99999;     // コストの最大値
const int MIN_VALUE = -9999999;  // 最小値
const int MAX_VALUE = 9999999;   // 最小値

// 座標計算で使用する配列
const int dy[5] = {0,-1, 1, 0, 0};
const int dx[5] = {0, 0, 0,-1, 1};

// その他
const int UNDEFINED = -1;           // 未定
const int REAL = true;              // 確定コマンド
const int DANGER_LINE = 50;         // 敵との距離での危険値
const int MY = 0;                   // 自軍ID
const int ENEMY = 1;                // 敵軍ID
bool firstPlayer = false;           // 1P側かどうか
int attackCount = 0;                // 突撃した回数
int createLimit = 40;               // 生産に関係する制限
int workerLimit = 28;               // 最初どのターンまで城からワーカを生産するか
int enemyCastelUnitId = UNDEFINED;  // 敵の城のID
int mostDownCoordY  = UNDEFINED;    // 一番下に探索したy座標
int mostRightCoordX = UNDEFINED;    // 一番右に探索したx座標

// 各ユニットへの命令
const char instruction[OPERATION_MAX] = {'X','U','D','L','R','0','1','2','3','4','5','6'};
// 各ユニットの生産にかかるコスト(上の「ユニット一覧」と一致させておく)
const int unitCost[UNIT_MAX] = {40, 20, 40, 60, COST_MAX, 100, 500};
// 各ユニットのHP
const int unitHp[UNIT_MAX] = {2000, 5000, 5000, 5000, 50000, 20000, 20000};
// 各ユニットの攻撃範囲
const int unitAttackRange[UNIT_MAX] = {2, 2, 2, 2, 10, 2, 2};
// 各ユニットの視野
const int unitEyeRange[UNIT_MAX] = {4, 4, 4, 4, 10, 10, 4};
// 各ユニットの行動の可否
const int unitCanMove[UNIT_MAX] = {true, true, true, true, false, false, false};

const int MAX_UNIT_ID   = 20010;  // ユニットのIDの上限
const int HEIGHT        = 100;    // フィールドの横幅
const int WIDTH         = 100;    // フィールドの縦幅

int manhattanDist[WIDTH*WIDTH];   // マンハッタン距離の出力
int reverseCoordTable[WIDTH];     // 座標系を逆にするためのテーブル
int enemyAI = UNDEFINED;
int maybeAI[9];                   // 敵のAI予測

// プレイヤーの名前
const string PLAYER_NAME = "siman";

// ダメージテーブル [攻撃する側][攻撃される側]
const int DAMAGE_TABLE[7][7] = {
  /*          労   騎   闘   殺   城   村   拠 */
  /* 労 */ { 100, 100, 100, 100, 100, 100, 100}, 
  /* 騎 */ { 100, 500, 200, 200, 200, 200, 200}, 
  /* 闘 */ { 500,1600, 500, 200, 200, 200, 200},
  /* 殺 */ {1000, 500,1000, 500, 200, 200, 200}, 
  /* 城 */ { 100, 100, 100, 100, 100, 100, 100}, 
  /* 村 */ { 100, 100, 100, 100, 100, 100, 100},
  /* 拠 */ { 100, 100, 100, 100, 100, 100, 100}
};

// 各ユニットが出来る行動 [ユニットID][行動リスト]
const bool OPERATION_LIST[UNIT_MAX][OPERATION_MAX] = {
  /*        動無   動上   動下   動左   動右   産労   産騎   産闘   産殺   産城   産村   産拠 */
  /* 労 */ {true,  true,  true,  true,  true, false, false, false, false, false,  true,  true},
  /* 騎 */ {true,  true,  true,  true,  true, false, false, false, false, false, false, false},
  /* 闘 */ {true,  true,  true,  true,  true, false, false, false, false, false, false, false},
  /* 殺 */ {true,  true,  true,  true,  true, false, false, false, false, false, false, false},
  /* 城 */ {true, false, false, false, false,  true, false, false, false, false, false, false},
  /* 村 */ {true, false, false, false, false,  true, false, false, false, false, false, false},
  /* 拠 */ {true, false, false, false, false, false,  true,  true,  true, false, false, false}
};

// ユニットへの指示
struct Operation{
  int unitId;       // ユニットID
  int operation;    // 命令のリスト
  int evaluation;   // 命令の評価値

  bool operator >(const Operation &e) const{
    return evaluation < e.evaluation;
  }    
};

// 最良探索で使用する
struct Cell{
  int y;
  int x;
  int cost;
  int originDirect;

  Cell(int ypos, int xpos, int c = -1, int od = UNDEFINED){
    y = ypos;
    x = xpos;
    cost = c;
    originDirect = od;
  }

  bool operator >(const Cell &e) const{
    return cost > e.cost;
  }    
};

// 行動の優先順位
struct MovePriority{
  int unitId;   // ユニットID
  int priority; // 優先度

  MovePriority(int id = 0, int value = 0){
    unitId = id;
    priority = value;
  }

  bool operator >(const MovePriority &e) const{
    return priority < e.priority;
  }    
};

// ユニットの数
struct UnitCount{
  int knightCount;
  int fighterCount;
  int assasinCount;
  int totalCount;

  UnitCount(){
    knightCount = 0;
    fighterCount = 0;
    assasinCount = 0;
    totalCount = 0;
  }
};

// フィールドの1マスに対応する
struct Node{
  bool resource;                        // 資源マスかどうか
  bool opened;                          // 調査予定マス
  bool searched;                        // 既に調査済みかどうか
  bool rockon;                          // ノードを狙っている自軍がいるかどうか
  bool nodamage;                        // ダメージを受けていない
  bool enemyCastel;                     // 敵の城がある可能性
  bool occupy;                          // 占領されているかどうかの確認
  int rockCount;                        // このノードを狙っているユニットの数
  int stamp;                            // 足跡
  int myK;                              // このマスから攻撃範囲(2)にいる自軍の数
  int enemyK;                           // このマスから攻撃範囲(2)にいる敵軍の数
  int cost;                             // ノードのコスト
  int markCount;                        // マークカウント(自軍が行動する予定のマス)
  int seenCount;                        // ノードを監視しているユニットの数 
  int troopsId;                         // 滞在中の軍隊ID
  int myUnitTotalCount;                 // 自軍の総数
  int myUnitCount[7];                   // 自軍の各ユニット数
  int enemyUnitTotalCount;              // 敵軍の総数
  int enemyUnitCount[7];                // 相手の各ユニット数
  int enemyAttackCount[7];              // 敵軍の攻撃の数
  int enemyCountWithinAttackRange[7];   // 攻撃範囲内にいる敵の種類
  int attackDamage[7];                  // 与えることの出来る攻撃力
  int receiveDamage[7];                 // 受けるダメージ
  int timestamp;                        // タイムスタンプ
  set<int> seenMembers;                 // ノードを監視している自軍のメンバー
  set<int> myUnits;                     // 自軍のIDリスト
  set<int> enemyUnits;                  // 敵軍のIDリスト
};

// ユニットが持つ属性
struct Unit{
  int id;                 // ユニットのID
  int mode;               // ユニットの状態
  int y;                  // y座標
  int x;                  // x座標
  int role;               // 役割
  int destY;              // 目的地のy座標
  int destX;              // 目的地のx座標
  int leaderId;           // 隊長のID
  int troopsCount;        // 部隊の人数
  int troopsLimit;        // 突撃する人数
  Node *townId;           // 拠点にしている村のID
  int resourceY;          // 目的地(資源)のy座標
  int resourceX;          // 目的地(資源)のx座標
  int createWorkerCount;  // 生産したワーカーの数
  int createKnightCount;  // 生産したナイトの数
  int createFighterCount; // ファイターを生産した数
  int createAssasinCount; // アサシンを生産した数
  int castelAttackCount;  // 敵の城から攻撃を受けた回数
  int beforeY;            // 前のターンのy座標
  int beforeX;            // 前のターンのx座標
  int hp;                 // HP
  int birthday;           // 生成されたターン
  int beforeHp;           // 前のターンのHP
  int type;               // ユニットの種別
  int currentOperation;   // このターンにとった行動
  int eyeRange;           // 視野
  int attackRange;        // 攻撃範囲
  bool movable;           // 移動できるかどうか
  bool moved;             // このターンの行動が完了したかどうか
  int timestamp;          // 更新ターン
};

// ゲーム・フィールド全体の構造
struct GameStage{
  int searchedNodeCount;                // 調査済みのマスの数
  int openedNodeCount;                  // 調査予定マスの数
  int visibleNodeCount;                 // 現在確保できている視界の数   
  int gameSituation;                    // 試合状況
  int incomeResource;                   // このターンに得られた収入
  int targetY;                          // 目的地
  int targetX;                          // 目的地
  int killedCount;                      // 倒された味方の数
  int currentEnemyUnitCount[UNIT_MAX];  // 遭遇した敵の数
  int baseCount;                        // 拠点の数
  int createVillageCount;               // このターンに立てた村の数
  bool castelAttack;                    // 城からの攻撃を受けている
  Node field[HEIGHT][WIDTH];            // ゲームフィールド
  queue<int> enemyCastelPointList;      // 敵の城の座標候補
};

// 座標を表す
struct Coord{
  int y;
  int x;
  int dist;

  Coord(int ypos = -1, int xpos = -1){
    y = ypos;
    x = xpos;
  }

  bool operator >(const Coord &e) const{
    return dist > e.dist;
  }    
};

typedef pair<Coord, int> cell;
typedef pair<int, int> kCount;

int remainingTime;              // 残り時間
int stageNumber;                // 現在のステージ数
int currentStageNumber;         // 現在のステージ数
int turn;                       // 現在のターン
int totalTurn;                  // ターンの合計値
int myAllUnitCount;             // 自軍のユニット数
int enemyAllUnitCount;          // 敵軍のユニット数
int resourceCount;              // 資源の数
int myResourceCount;            // 自軍の資源の数
int myCastelCoordY;             // 自軍の城のy座標
int myCastelCoordX;             // 自軍の城のx座標
int enemyCastelCoordY;          // 敵軍の城のy座標
int enemyCastelCoordX;          // 敵軍の城のx座標
int hitPointY;                  // 敵の城の攻撃を受けた座標
int hitPointX;                  // 敵の城の攻撃を受けた座標
Unit unitList[MAX_UNIT_ID];     // ユニットのリスト
set<int> myActiveUnitList;      // 生存している自軍のユニットのIDリスト
set<int> enemyActiveUnitList;   // 生存している敵軍のユニットのIDリスト
set<int> resourceNodeList;      // 資源マスのリスト
set<int> enemyResourceNodeList; // 敵の資源マスのリスト

bool walls[HEIGHT+2][WIDTH+2];      // 壁かどうかを確認するだけのフィールド
int fieldCost[HEIGHT][WIDTH];       // フィールドのコスト
int dangerPointList[HEIGHT][WIDTH]; // 危険度
map<int, bool> unitIdCheckList;     // IDが存在しているかどうかのチェック

GameStage gameStage;      // ゲームフィールド

/*
 * メインのコード部分
 */
class Codevs{
  public:
    /*
     * ゲームの初期化処理
     */
    void init(){
      currentStageNumber = -1;
      totalTurn = 0;
      firstPlayer = false;

      manhattanDistInitialize();
      reverseCoordTableInitialize();

      // 壁判定の初期化処理
      for(int y = 0; y <= HEIGHT+1; y++){
        for(int x = 0; x <= WIDTH+1; x++){
          walls[y][x] = (y == 0 || x == 0 || y == HEIGHT+1 || x == WIDTH + 1);
        }
      }

      // 一番最初でプレイヤー名の出力
      printf("%s\n", PLAYER_NAME.c_str());
    }

    /*
     * マンハッタン距離の初期化
     */
    void manhattanDistInitialize(){
      for(int y = 0; y < HEIGHT; y++){
        for(int x = 0; x < WIDTH; x++){
          int id = (y*WIDTH+x);
          int dist = abs(y-x);
          manhattanDist[id] = dist;
        } 
      }
    }

    /*
     * 座標系変換テーブルの初期化
     */
    void reverseCoordTableInitialize(){
      for(int i = 0; i < WIDTH; i++){
        reverseCoordTable[i] = 99 - i;
      }
    }

    /*
     * ステージ開始直前に行う初期化処理
     */
    void stageInitialize(){
      totalTurn += (turn == 0)? 0 : turn+1;
      /*
      fprintf(stderr,"total turn: %d\n", totalTurn);
      fprintf(stderr,"stageInitialize =>\n");
      */
      // ユニットのチェックリストの初期化
      unitIdCheckList.clear();

      // アクティブユニットリストの初期化
      myActiveUnitList.clear();

      // 敵ユニットリストの初期化
      enemyActiveUnitList.clear();

      // 行動の優先順位を初期化
      movePriority[WORKER] = 5;

      // 突撃回数の初期化
      attackCount = 0;
      
      // 味方の倒された数を初期化
      gameStage.killedCount = 0;

      // 資源マスの初期化
      resourceNodeList.clear();

      // 敵の城の座標候補リストの初期化
      queue<int> que;
      gameStage.enemyCastelPointList = que;

      // 拠点の数
      gameStage.baseCount = 0;

      // 城からの攻撃をfalse
      gameStage.castelAttack = false;

      // 最初に探索する部分を決める
      gameStage.targetY = 20;
      gameStage.targetX = 20;

      // 敵の城からの攻撃座標をリセット
      hitPointY = UNDEFINED;
      hitPointX = UNDEFINED;

      // 敵の遭遇回数をリセット
      memset(gameStage.currentEnemyUnitCount, 0, sizeof(gameStage.currentEnemyUnitCount));

      // 敵AIの予測をリセット
      memset(maybeAI, true, sizeof(maybeAI));

      // ゲームの初期状態
      gameStage.gameSituation = OPENING;

      // 探索が完了したマスの初期化
      gameStage.searchedNodeCount = 0;

      // 確保している視界の数の初期化
      gameStage.visibleNodeCount = 0;

      // 調査予定のノード数の初期化
      gameStage.openedNodeCount = 0;

      // 生産上限を初期化
      createLimit = (currentStageNumber <= 23)? 40 : 40;

      // ワーカの生産上限を初期化
      workerLimit = (currentStageNumber <= 23)? 20 : 28;

      // 2P側と仮定する
      firstPlayer = false;

      // 敵AIの予想をリセット
      enemyAI = UNDEFINED;

      // フィールドの初期化
      for(int y = 0; y < HEIGHT; y++){
        for(int x = 0; x < WIDTH; x++){
          gameStage.field[y][x] = createNode();
        }
      }


      // 自軍の城の座標をリセット
      myCastelCoordY = 0;
      myCastelCoordX = 0;

      // 敵軍の城の座標をリセット
      enemyCastelCoordY = UNDEFINED;
      enemyCastelCoordX = UNDEFINED;
      enemyCastelUnitId = UNDEFINED;
    }

    /*
     * 各ターンの入力処理と初期化処理
     */
    void eachTurnProc(){
      int unitId;   // ユニットID
      int y;        // y座標
      int x;        // x座標
      int hp;       // HP
      int unitType; // ユニットの種類
      Coord coord;  // 変換後の座標
      string str;   // 終端文字列「END」を格納するだけの変数

      // 調査予定のノード数をリセット
      gameStage.openedNodeCount = 0;

      // 確保できている視野のリセット
      gameStage.visibleNodeCount = 0;

      // 立てた村の数をリセット
      gameStage.createVillageCount = 0;

      // 現在のステージ数(0-index)
      scanf("%d", &stageNumber);

      /* 
       * 今のステージ数と取得したステージ数が異なる場合は
       * 新規ステージなので初期化を行う
       */
      if(stageNumber != currentStageNumber){
        stageInitialize();
        currentStageNumber = stageNumber;
      }

      // 現在のターン数(0-index)
      scanf("%d", &turn);

      if(currentStageNumber == 40 && turn % 20 == 0){
        fprintf(stderr,"totalTurn = %d\n", totalTurn + turn);
      }

      // 資源数
      scanf("%d", &myResourceCount);

      // 自軍のユニット数
      scanf("%d", &myAllUnitCount);

      // 自軍ユニットの詳細
      for(int i = 0; i < myAllUnitCount; i++){
        scanf("%d %d %d %d %d", &unitId, &y, &x, &hp, &unitType);

        // 自軍の城の座標を更新
        if(unitType == CASTEL){
          myCastelCoordY = y;
          myCastelCoordX = x;
        }


        firstPlayer = isFirstPlayer();
        if(turn == 0 && unitType == CASTEL){
          if(firstPlayer){
            //fprintf(stderr,"stage = %d, first player!\n", currentStageNumber);
          }else{
            //fprintf(stderr,"stage = %d, second player!\n", currentStageNumber);
          }
        }
        coord = reverseCoord(y,x);


        // チェックリストに載っていない場合は、新しくユニットのデータを生成する
        if(!unitIdCheckList[unitId]){
          addMyUnit(unitId, coord.y, coord.x, hp, unitType);
        }else{
          updateMyUnitStatus(unitId, coord.y, coord.x, hp);
        }
      }


      // 視野内の敵軍のユニット数
      scanf("%d", &enemyAllUnitCount);

      // 敵軍ユニットの詳細
      for(int i = 0; i < enemyAllUnitCount; i++){
        scanf("%d %d %d %d %d", &unitId, &y, &x, &hp, &unitType);

        coord = reverseCoord(y,x);

        if(unitType == VILLAGE){
          maybeAI[SILVER] = false;
        }
        
        // チェックリストに載っていない場合は、新しくユニットのデータを生成する
        if(!unitIdCheckList[unitId]){
          addEnemyUnit(unitId, coord.y, coord.x, hp, unitType);
        }else{
          updateEnemyUnitStatus(unitId, coord.y, coord.x, hp);
        }
      }

      // 視野内の資源の数
      scanf("%d", &resourceCount);

      // 資源マスの詳細
      for(int i = 0; i < resourceCount; i++){
        scanf("%d %d", &y, &x);
        coord = reverseCoord(y,x);
        addResourceNode(coord.y,coord.x);
      }

      // 終端文字列
      cin >> str;
    }

    /*
     * 自軍ユニットの追加を行う
     *   unitId: ユニットID
     *        y: y座標
     *        x: x座標
     *       hp: HP
     * unitType: ユニットの種類
     */
    void addMyUnit(int unitId, int y, int x, int hp, int unitType){
      Unit unit;
      unit.id                 = unitId;
      unit.y                  = y;
      unit.x                  = x;
      unit.hp                 = hp;
      unit.beforeHp           = hp;
      unit.type               = unitType;
      unit.destY              = UNDEFINED;
      unit.destX              = UNDEFINED;
      unit.resourceY          = UNDEFINED;
      unit.resourceX          = UNDEFINED;
      unit.createWorkerCount  = 0;
      unit.createKnightCount  = 0;
      unit.castelAttackCount  = 0;
      unit.leaderId           = UNDEFINED;
      unit.troopsCount        = 0;
      unit.troopsLimit        = MAX_VALUE;
      unit.attackRange        = unitAttackRange[unitType];
      unit.eyeRange           = unitEyeRange[unitType];
      unit.movable            = unitCanMove[unitType];
      unit.moved              = false;
      unit.birthday           = turn;
      unit.timestamp          = turn;

      // 自軍の城の座標を更新
      if(unitType == CASTEL){
        myCastelCoordY = y;
        myCastelCoordX = x;
      }

      Node *node = getNode(y,x);

      node->myUnitTotalCount += 1;
      node->myUnitCount[unitType] += 1;
      node->myUnits.insert(unitId);

      unitList[unitId] = unit;
      unitList[unitId].mode = directFirstMode(&unitList[unitId]);
      unitList[unitId].role = directUnitRole(&unitList[unitId]);
      myActiveUnitList.insert(unitId);
      unitIdCheckList[unitId] = true;
      checkNode(unitId, y, x, unit.eyeRange);

      // 戦闘員の場合は、近くにいるリーダを探す
      if(unitList[unitId].role == COMBATANT){
        searchLeader(&unitList[unitId]);

        assert(unitList[unitId].leaderId >= 0);
      }
    }

    /*
     * リーダを探し出す
     */
    void searchLeader(Unit *unit){
      set<int>::iterator id = myActiveUnitList.begin();
      int minDist = MAX_VALUE;
      int dist;
      int leaderId = UNDEFINED;

      while(id != myActiveUnitList.end()){
        assert(*id >= 0 && *id <= 20000);
        Unit *other = getUnit(*id);
        assert(other->y >= 0 && other->x >= 0 && other->y < HEIGHT && other->x < WIDTH);
        dist = calcManhattanDist(unit->y, unit->x, other->y, other->x);

        if(other->role == LEADER && minDist > dist){
          minDist = dist;
          leaderId = other->id;
        }

        id++;
      }

      unit->leaderId = leaderId;
      unitList[leaderId].troopsCount += 1;
    }

    /*
     * 自軍のユニットを削除する
     * unit: ユニット情報
     */
    void removeMyUnit(Unit *unit){
      myActiveUnitList.erase(unit->id);
      uncheckNode(unit->y, unit->x, unit->type, unit->eyeRange);
      gameStage.killedCount += 1;

      if(unit->type == BASE){
        gameStage.baseCount -= 1;
      }
      if(unit->role == LEADER && unit->troopsLimit > 1){
        switchLeader(unit);
      }
      if(unit->role == COMBATANT){
        assert(unit->leaderId >= 0 && unit->leaderId <= 20000);
        Unit *leader = getUnit(unit->leaderId);
        leader->troopsCount -= 1;
      }
    } 

    /*
     * リーダがいなくなった場合に新しいリーダにスイッチする
     */
    void switchLeader(Unit *leader){
      set<int>::iterator id = myActiveUnitList.begin();
      int newLeaderId = searchNextLeader(leader);

      assert(newLeaderId >= 0);

      while(id != myActiveUnitList.end()){
        assert(*id >= 0 && *id <= 20000);
        Unit *unit = getUnit(*id);

        if(unit->id == newLeaderId){
          unit->role = LEADER;
          unit->troopsLimit = leader->troopsLimit;
          unit->troopsCount = leader->troopsCount;
        }else if(unit->leaderId == leader->id){
          unit->leaderId = newLeaderId;
        }

        id++;
      }
    }

    /*
     * 次のリーダの探索を行う。リーダが倒された時には、次の一番近いユニットをリーダとする
     */
    int searchNextLeader(Unit *leader){
      int minDist = MAX_VALUE;
      int leaderId = 0;
      int dist;

      set<int>::iterator it = myActiveUnitList.begin();

      while(it != myActiveUnitList.end()){
        assert(*it >= 0);
        Unit *unit = getUnit(*it);

        if(unit->id != leader->id && unit->movable){
          assert(leader->y >= 0 && leader->x >= 0 && leader->y < HEIGHT && leader->x < WIDTH);
          dist = calcManhattanDist(unit->y, unit->x, leader->y, leader->x);

          if(minDist > dist && unit->timestamp == turn){
            minDist = dist;
            leaderId = unit->id;
          }
        }

        it++;
      }

      return leaderId;
    }

    /*
     * 発見している資源マスが既に生産限界に来ているかどうか
     */
    bool isResourceFull(){
      set<int>::iterator it = resourceNodeList.begin();

      while(it != resourceNodeList.end()){
        assert(*it >= 0 && *it < 10000);
        int y = (*it)/WIDTH;
        int x = (*it)%WIDTH;
        Node *node = getNode(y,x);

        if(!isOccupied(y,x)){
          if(node->myUnitCount[WORKER] < 5) return false;
        }

        it++;
      }

      return true;
    }

    /*
     * 既に生産上限に達しているかどうか
     */
    bool isCreateLimitReach(){
      set<int>::iterator it = resourceNodeList.begin();
      int rockOnCount = 0;

      while(it != resourceNodeList.end()){
        assert(*it >= 0 && *it < 10000);
        int y = (*it)/WIDTH;
        int x = (*it)%WIDTH;
        Node *node = getNode(y,x);

        rockOnCount += node->rockon;

        it++;
      }

      return 10 + rockOnCount * 5 >= createLimit;
    }

    /*
     * 敵軍のユニットを削除する
     */
    void removeEnemyUnit(Unit *unit){
      assert(unit->id >= 0 && unit->id <= 20000);
      enemyActiveUnitList.erase(unit->id);
      gameStage.currentEnemyUnitCount[unit->type] -= 1;
      unitIdCheckList[unit->id] = false;
    }

    /*
     * 敵軍ユニットの追加を行う
     *   unitId: ユニットID
     *        y: y座標
     *        x: x座標
     *       hp: HP
     * unitType: ユニットの種類
     */
    void addEnemyUnit(int unitId, int y, int x, int hp, int unitType){
      Unit unit;
      unit.id           = unitId;
      unit.y            = y;
      unit.x            = x;
      unit.hp           = hp;
      unit.type         = unitType;
      unit.attackRange  = unitAttackRange[unitType];
      unit.eyeRange     = unitEyeRange[unitType];
      unit.movable      = unitCanMove[unitType];
      unit.timestamp    = turn;

      Node *node = getNode(y, x);

      // 敵軍の城の座標を更新
      if(unitType == CASTEL){
        enemyCastelCoordY = y;
        enemyCastelCoordX = x;
        enemyCastelUnitId = unitId;

        // 城が見つかったら予測座標は全て廃棄
        queue<int> que;
        gameStage.enemyCastelPointList = que;
      }

      gameStage.currentEnemyUnitCount[unitType] += 1;
      node->enemyUnitCount[unitType] += 1;
      node->enemyUnitTotalCount += 1;
      node->enemyUnits.insert(unitId);
      unitList[unitId] = unit;
      enemyActiveUnitList.insert(unitId);
      unitIdCheckList[unitId] = true;
    }

    /*
     * 資源マスの追加を行う
     */    
    void addResourceNode(int y, int x){
      assert(y >= 0 && x >= 0 && y < HEIGHT && x < WIDTH);
      gameStage.field[y][x].resource = true;
      resourceNodeList.insert(y*WIDTH+x);
    }

    /*
     * 資源マスが占領されているかどうかを調べる
     *   - Workerが1体以上
     *   - 村が1つ以上
     * いずれかを満たす場合は占領されていると判断
     */
    bool isOccupied(int y, int x){
      assert(y >= 0 && x >= 0 && y < HEIGHT && x < WIDTH);
      Node *node = getNode(y, x);
      return (node->resource && (node->enemyUnitCount[WORKER] > 0 || node->enemyUnitCount[VILLAGE] > 0));
    }

    /*
     * 資源マスの情報について更新
     */
    bool updateResourceNodeData(){
      set<int>::iterator it = resourceNodeList.begin();

      while(it != resourceNodeList.end()){
        assert(*it >= 0 && *it < 10000); 
        int y = (*it)/WIDTH;
        int x = (*it)%WIDTH;

        if(isOccupied(y,x)){
          enemyResourceNodeList.insert(y*WIDTH+x);
        }else{
          enemyResourceNodeList.erase(y*WIDTH+x);
        }

        it++;
      }
    }

    /*
     * 敵が占領している資源マスで近い場所を調べる
     *   ypos: y座標
     *   xpos: x座標
     */
    Coord searchEnemyOccupiedResourceNode(int ypos, int xpos){
      Coord coord;
      int minDist = MAX_VALUE;
      int dist;

      assert(ypos >= 0 && xpos >= 0 && ypos < HEIGHT && xpos < WIDTH);

      set<int>::iterator it = enemyResourceNodeList.begin();

      while(it != enemyResourceNodeList.end()){
        assert(*it >= 0 && *it < 10000); 
        int y = (*it)/WIDTH;
        int x = (*it)%WIDTH;

        assert(y >= 0 && x >= 0 && y < HEIGHT && x < WIDTH);
        dist = calcManhattanDist(ypos, xpos, y, x);

        if(minDist > dist){
          minDist = dist;
          coord.y = y;
          coord.x = x;
        }

        it++;
      }

      return coord;
    }

    /*
     * 戦闘に必要な情報を更新
     */
    void updateBattleData(){
      for(int y = 0; y < HEIGHT; y++){
        for(int x = 0; x < WIDTH; x++){
          Node *node = getNode(y,x);

          updateNodeData(y,x);

          if(node->enemyK > 0 && node->myK > 0){
            for(int i = 0; i < 4; i++){
              for(int j = 0; j < UNIT_MAX; j++){
                node->attackDamage[i] += node->enemyCountWithinAttackRange[j] * (DAMAGE_TABLE[i][j] / node->enemyK);
                node->receiveDamage[i] += node->enemyCountWithinAttackRange[j] * (DAMAGE_TABLE[j][i] / node->myK);
              }
            }
          }
        }
      }
    }

    /*
     * 攻撃範囲にいる敵の数とKの値を更新
     */
    void updateNodeData(int y, int x, int range = 2){
      Node *originNode = getNode(y,x);

      map<int, bool> checkList;
      queue<cell> que;
      que.push(cell(Coord(y, x), 0));

      while(!que.empty()){
        cell c = que.front(); que.pop(); 
        Coord coord = c.first;
        int dist = c.second;

        if(checkList[coord.y*WIDTH+coord.x] || dist > range) continue;
        checkList[coord.y*WIDTH+coord.x] = true;

        Node *node = getNode(coord.y, coord.x);
        originNode->myK += min(10, node->myUnitTotalCount);
        originNode->enemyK += min(10, node->enemyUnitTotalCount);
        originNode->enemyCountWithinAttackRange[WORKER] += node->enemyUnitCount[WORKER];
        originNode->enemyCountWithinAttackRange[KNIGHT] += node->enemyUnitCount[KNIGHT];
        originNode->enemyCountWithinAttackRange[FIGHTER] += node->enemyUnitCount[FIGHTER];
        originNode->enemyCountWithinAttackRange[ASSASIN] += node->enemyUnitCount[ASSASIN];

        for(int i = 1; i < 5; i++){
          int ny = coord.y + dy[i];
          int nx = coord.x + dx[i];
          if(!isWall(ny,nx)) que.push(cell(Coord(ny, nx), dist+1));
        }
      }
    }

    /*
     * 突撃する人数を変える
     */ 
    void updateTroopsLimit(Unit *unit, bool check = true){
      // 城が見つかっている場合
      if(isEnemyCastelDetected()){
        Node *node = getNode(enemyCastelCoordY, enemyCastelCoordX);

        if(!isLila(check) && isEnemyCastelSpy() && node->enemyUnitCount[BASE] <= 0){
          unit->troopsLimit = min(unit->troopsLimit, 20);
        }else if(isLila(check) && attackCount <= 1){
          unit->troopsLimit = max(unit->troopsLimit, 40);
        }else if(isGrun(check) && attackCount <= 1){
          unit->troopsLimit = max(unit->troopsLimit, 40);
        }
      }else{
        if(isChokudai(check)){
          unit->troopsLimit = min(unit->troopsLimit, 20);
        }else if(isLila(check) && attackCount <= 1){
          unit->troopsLimit = max(unit->troopsLimit, 40);
        }else if(isGrun(check) && attackCount <= 1){
          unit->troopsLimit = max(unit->troopsLimit, 40);
        }
      }
    }

    /*
     * ユニットの最初のモードを決める
     *   SEARCH   - 探索部隊
     *   PICKING  - 資源回収部隊
     *   DESTROY  - 戦闘モード
     */
    int directFirstMode(Unit *unit){
      Node *node = getNode(unit->y, unit->x);
      updateTroopsLimit(unit, false);

      switch(unit->type){
        case WORKER:
          if(node->resource && node->myUnitCount[WORKER] <= 5 && unit->birthday > 20){
            unit->resourceY = unit->y;
            unit->resourceX = unit->x;
            unit->destY = unit->y;
            unit->destX = unit->x;

            return PICKING;
          }else if(unit->id == 1 || unit->id == 2){
          //}else if(unit->id == 1){
          //}else if(unit->birthday == 5 || unit->birthday == 9){
            return SPY;
          }else{
            return SEARCH;
          }
          break;
        case VILLAGE:
          return NONE;
          break;
        case KNIGHT:
          return SEARCH;
          break;
        case FIGHTER:
          if(unit->troopsCount < unit->troopsLimit){
            return STAY;
          }else{
            return DESTROY;
          }
          break;
        case ASSASIN:
          if(unit->troopsCount < unit->troopsLimit){
            return STAY;
          }else{
            return DESTROY;
          }
          break;
        default:
          break;
      }

      return NONE;
    }

    /*
     * 次の目的地を決める(指定したポイントから一番近い未探索地域)
     */
    Coord directNextPoint(Unit *unit){
      Coord bestCoord;

      if(gameStage.gameSituation == ONRUSH){
        assert(isEnemyCastelDetected());
      }

      queue<Coord> que;
      que.push(Coord(unit->y, unit->x));
      map<int, bool> checkList;

      while(!que.empty()){
        Coord coord = que.front(); que.pop();

        if(checkList[coord.y*WIDTH+coord.x]) continue;
        checkList[coord.y*WIDTH+coord.x] = true;

        Node *node = getNode(coord.y, coord.x);

        if(!node->searched && node->markCount == 0){
          return coord;
        }

        for(int i = 1; i < 5; i++){
          int ny = coord.y + dy[i];
          int nx = coord.x + dx[i];

          if(!isWall(ny,nx)) que.push(Coord(ny,nx));
        }
      }

      assert(enemyCastelCoordY >= 0 && enemyCastelCoordX >= 0 && enemyCastelCoordY < HEIGHT && enemyCastelCoordX < WIDTH);
      return Coord(enemyCastelCoordY, enemyCastelCoordX);
    }

    /*
     * 収入を更新
     */
    void updateIncomeResource(){
      gameStage.incomeResource = 10;
      set<int>::iterator it = resourceNodeList.begin();

      while(it != resourceNodeList.end()){
        assert(*it >= 0 && *it < 10000);
        int y = (*it)/WIDTH;
        int x = (*it)%WIDTH;

        Node *node = getNode(y, x);

        gameStage.incomeResource += min(5, node->myUnitCount[WORKER]);

        it++;
      }
    }

    /*
     * 敵AIの予測
     */
    void checkEnemyAI(){
      if(isEnemyCastelDetected() && isEnemyCastelSpy()){
        Node *enemyCastel = getNode(enemyCastelCoordY, enemyCastelCoordX);

        // 城に拠点を作らないAI達
        if(enemyCastel->enemyUnitCount[BASE] > 0){
          maybeAI[CHOKUDAI] = false;
          maybeAI[COLUN] = false;
          maybeAI[ROSA] = false;
          maybeAI[SILVER] = false;
        }
      }

      if(isGrun()){
        enemyAI = GRUN;
      }else if(isChokudai()){
        enemyAI = CHOKUDAI;
      }else if(isSilver()){
        enemyAI = SILVER;
      }else if(isLila()){
        enemyAI = LILA;
      }
    }

    /*
     * 次の未開地を決める
     */
    void updateUnknownPoint(){
      Coord bestCoord;
      assert(gameStage.targetY >= 0 && gameStage.targetX >= 0 && gameStage.targetY < HEIGHT && gameStage.targetX < WIDTH);
      Node *node = getNode(gameStage.targetY, gameStage.targetX);

      // 未開の地がまだ未探索の場合はそのまま
      if(!gameStage.castelAttack && !node->searched){
        return;
      }else if(!gameStage.castelAttack && gameStage.targetY + 10 < 99 && gameStage.targetX + 10 < 99){
        gameStage.targetY += 10;
        gameStage.targetX += 10;
        return;
      }else if(gameStage.castelAttack && gameStage.enemyCastelPointList.size() > 0){
        Node *node = getNode(gameStage.targetY, gameStage.targetX);

        if(node->searched || !node->enemyCastel){
          gameStage.enemyCastelPointList.pop();
        }else{
          return;
        }

        fprintf(stderr,"enemyCastelPointSize = %lu\n", gameStage.enemyCastelPointList.size());
        assert(gameStage.enemyCastelPointList.size() > 0);
        int id = gameStage.enemyCastelPointList.front();
        assert(id >= 0 && id < 10000);
        int y = id / WIDTH;
        int x = id % WIDTH;

        gameStage.targetY = y;
        gameStage.targetX = x;

        return;
      }else if(isEnemyCastelDetected()){
        gameStage.targetY = enemyCastelCoordY;
        gameStage.targetX = enemyCastelCoordX;
      }else{
        if(gameStage.gameSituation == ONRUSH){
          assert(isEnemyCastelDetected());
        }

        queue<Coord> que;
        que.push(Coord(gameStage.targetY, gameStage.targetX));
        map<int, bool> checkList;

        while(!que.empty()){
          Coord coord = que.front(); que.pop();

          if(checkList[coord.y*WIDTH+coord.x]) continue;
          checkList[coord.y*WIDTH+coord.x] = true;

          Node *node = getNode(coord.y, coord.x);

          assert(coord.y >= 0 && coord.x >= 0);
          if(node->enemyCastel && calcManhattanDist(coord.y, coord.x, 99, 99) <= 40){
            gameStage.targetY = coord.y;
            gameStage.targetX = coord.x;
            return;
          }

          for(int i = 1; i < 5; i++){
            int ny = coord.y + dy[i];
            int nx = coord.x + dx[i];

            if(!isWall(ny,nx)) que.push(Coord(ny,nx));
          }
        }

        assert(enemyCastelCoordY >= 0 && enemyCastelCoordX >= 0 && enemyCastelCoordY < HEIGHT && enemyCastelCoordX < WIDTH);

        gameStage.targetY = enemyCastelCoordY;
        gameStage.targetX = enemyCastelCoordX;
      }
    }

    /*
     * 目的地を決めて探索する
     */
    Coord directTargetPoint(int y, int x, int targetY, int targetX){
      map<int, bool> checkList;
      priority_queue< Cell, vector<Cell>, greater<Cell> > que;

      assert(targetY >= 0 && targetX >= 0 && targetY < HEIGHT && targetX < WIDTH);

      que.push(Cell(y, x, 0));

      while(!que.empty()){
        Cell cell = que.top(); que.pop();

        if(checkList[cell.y*WIDTH+cell.x]) continue;
        checkList[cell.y*WIDTH+cell.x] = true;

        Node *node = getNode(cell.y, cell.x);

        if(!node->searched && node->markCount == 0){
          return Coord(cell.y, cell.x);
        }

        for(int i = 1; i < 5; i++){
          int ny = cell.y + dy[i];
          int nx = cell.x + dx[i];

          if(!isWall(ny,nx)){
            int cost = node->stamp + node->cost;
            int dist = calcManhattanDist(ny, nx, targetY, targetX);

            if(cell.cost == 0){
              que.push(Cell(ny, nx, dist + cell.cost + cost, i));
            }else{
              que.push(Cell(ny, nx, dist + cell.cost + cost, cell.originDirect));
            }
          }
        }
      }

      return Coord(UNDEFINED, UNDEFINED);
    };

    /*
     * 次に行動する方向を決める(なるべく自軍と被らず敵を避けるように)
     */
    int getNextDirection(int ypos, int xpos, int destY, int destX){
      map<int, bool> openNodeList;
      priority_queue< Cell, vector<Cell>, greater<Cell> > que;

      assert(destY >= 0 && destX >= 0 && destY < HEIGHT && destX < WIDTH);

      que.push(Cell(ypos, xpos, 0));

      while(!que.empty()){
        Cell cell = que.top(); que.pop();

        if(openNodeList[cell.y*WIDTH+cell.x]) continue;
        openNodeList[cell.y*WIDTH+cell.x] = true;

        Node *node = getNode(cell.y, cell.x);

        if(cell.y == destY && cell.x == destX){
          return cell.originDirect;
        }

        for(int i = 1; i < 5; i++){
          int ny = cell.y + dy[i];
          int nx = cell.x + dx[i];

          if(!isWall(ny,nx)){
            int cost = node->stamp + node->cost + dangerPointList[ny][nx];
            int dist = calcManhattanDist(ny, nx, destY, destX);
            int centerDist = calcManhattanDist(ny, nx, 50, 50);

            if(cell.originDirect == UNDEFINED){
              que.push(Cell(ny, nx, dist - centerDist + cell.cost + cost, i));
            }else{
              que.push(Cell(ny, nx, dist - centerDist + cell.cost + cost, cell.originDirect));
            }
          }
        }
      }

      return NO_MOVE;
    }

    /*
     * ノードの作成を行う
     */
    Node createNode(){
      Node node;
      memset(node.myUnitCount, 0, sizeof(node.myUnitCount));
      memset(node.enemyUnitCount, 0, sizeof(node.enemyUnitCount));
      memset(node.enemyAttackCount, 0, sizeof(node.enemyAttackCount));
      memset(node.enemyCountWithinAttackRange, 0, sizeof(node.enemyCountWithinAttackRange));
      memset(node.attackDamage, 0, sizeof(node.attackDamage));
      memset(node.receiveDamage, 0, sizeof(node.receiveDamage));
      node.seenCount            = 0;
      node.cost                 = 0;
      node.stamp                = 0;
      node.myK                  = 0;
      node.enemyK               = 0;
      node.myUnitTotalCount     = 0;
      node.rockCount            = 0;
      node.enemyUnitTotalCount  = 0;
      node.markCount            = 0;
      node.timestamp            = 0;
      node.troopsId             = UNDEFINED;
      node.resource             = false;
      node.opened               = false;
      node.rockon               = false;
      node.searched             = false;
      node.nodamage             = false;
      node.occupy               = false;
      node.enemyCastel          = true;

      return node;
    }

    /*
     * ユニットの作成を行う
     *        y: y座標
     *        x: x座標
     * unitType: 生産するユニットの種類
     */
    void createUnit(int y, int x, int unitType){
      gameStage.field[y][x].myUnitCount[unitType] += 1;
      myResourceCount -= unitCost[unitType];
      openNode(y, x, unitEyeRange[unitType]);
    }

    /*
     * ユニットの削除を行う
     *        y: y座標
     *        x: x座標
     * unitType: ユニットの種類
     */
    void deleteUnit(int y, int x, int unitType){
      gameStage.field[y][x].myUnitCount[unitType] -= 1;
      myResourceCount += unitCost[unitType];
      closeNode(y, x, unitEyeRange[unitType]);
    }

    /*
     * ノードのタイムスタンプを更新する
     */
    void updateNodeTimestamp(int y, int x, int range = 4){
      map<int, bool> checkList;
      queue<cell> que;
      que.push(cell(Coord(y, x), 0));

      while(!que.empty()){
        cell c = que.front(); que.pop();

        Coord coord = c.first;
        int dist = c.second;

        if(checkList[coord.y*WIDTH+coord.x] || dist > range) continue;
        checkList[coord.y*WIDTH+coord.x] = true;

        Node *node = getNode(coord.y, coord.x);
        node->timestamp = turn;

        for(int i = 1; i < 5; i++){
          int ny = coord.y + dy[i];
          int nx = coord.x + dx[i];

          if(!isWall(ny,nx)) que.push(cell(Coord(ny,nx), dist+1));
        }
      }
    }

    /*
     * 自軍ユニットの状態の更新を行う(座標と残りHP)
     * unitId: ユニットのID
     *      y: y座標
     *      x: x座標
     *     hp: HP
     */
    void updateMyUnitStatus(int unitId, int y, int x, int hp){
      assert(unitId >= 0 && unitId <= 20000);
      Unit *unit      = getUnit(unitId);
      unit->y         = y;
      unit->x         = x;
      unit->beforeHp  = unit->hp;
      unit->hp        = hp;
      unit->moved     = false;
      unit->timestamp = turn;

      Node *node = getNode(y, x);

      if(!node->nodamage && unit->beforeHp == unit->hp){
        node->nodamage = true; 
        checkNoEnemyCastel(y,x);
      }

      node->myUnitTotalCount += 1;
      node->myUnitCount[unit->type] += 1;
      node->myUnits.insert(unitId);
    }

    /*
     * 敵軍ユニットの状態の更新を行う(座標と残りHP)
     * unitId: ユニットのID
     *      y: y座標
     *      x: x座標
     *     hp: HP
     */
    void updateEnemyUnitStatus(int unitId, int y, int x, int hp){
      assert(unitId >= 0 && unitId <= 20000);
      Unit *unit      = getUnit(unitId);
      unit->y         = y;
      unit->x         = x;
      unit->hp        = hp;
      unit->timestamp = turn;

      Node *node = getNode(y, x);

      node->enemyUnits.insert(unitId);
      node->enemyUnitTotalCount += 1;
      node->enemyUnitCount[unit->type] += 1;
    }

    /*
     * ユニットのモードの状態の更新を行う
     */
    void updateUnitMode(){
      int aroundMyCastelUnitCount = aroundMyUnitCount(myCastelCoordY, myCastelCoordX, 1).totalCount;
      set<int>::iterator it = myActiveUnitList.begin();

      while(it != myActiveUnitList.end()){
        assert(*it >= 0);
        Unit *unit = getUnit(*it);
        unit->mode = directUnitMode(unit);

        if(unit->role == GUARDIAN && aroundMyCastelUnitCount >= 40 && unit->y == myCastelCoordY && unit->x == myCastelCoordX){
          unit->role = LEADER;
          unit->troopsLimit = 1;
          unit->troopsCount = 1;
          unit->mode = DESTROY;
        }

        it++;
      }
    }

    /*
     * 各ユニットの目的地を決める
     */
    void updateUnitDestination(){
      if(gameStage.gameSituation != ONRUSH){
        updateUnknownPoint();
      }

      set<int>::iterator it = myActiveUnitList.begin();

      while(it != myActiveUnitList.end()){
        assert(*it >= 0);
        Unit *unit = getUnit(*it);

        // SEARCHモードのユニットの目的地の設定されていない場合、更新する。
        if((unit->mode == SEARCH) && (gameStage.field[unit->destY][unit->destX].searched || (unit->destY == UNDEFINED && unit->destX == UNDEFINED))){
          updateSeacherDestination(unit);

          checkMark(unit->destY, unit->destX);
          assert(unit->destY >= 0 && unit->destX >= 0 && unit->destY < HEIGHT && unit->destX < WIDTH);
        }else if(unit->mode == RESOURCE_BREAK){
          updateVillageBreakerDestination(unit);
        }

        if(unit->mode == SEARCH){
          assert(unit->destY >= 0 && unit->destX >= 0 && unit->destY < HEIGHT && unit->destX < WIDTH);
        }
        if(unit->role == LEADER){
          updateSeacherDestination(unit);
        }

        it++;
      }
    }

    /*
     * 探索部隊の目的地を更新する
     */
    void updateSeacherDestination(Unit *seacher){
      if(gameStage.gameSituation == ONRUSH){
        seacher->destY = enemyCastelCoordY;
        seacher->destX = enemyCastelCoordX;
      // 序盤は自分のワーカと被らないように探索を行う
      }else if(gameStage.gameSituation == OPENING){
        Coord coord = directTargetPoint(seacher->y, seacher->x, gameStage.targetY, gameStage.targetX);
        seacher->destY = coord.y;
        seacher->destX = coord.x;
      // 敵の城が見つかっている場合は敵の城を目指す
      }else if(isEnemyCastelDetected()){
        seacher->destY = enemyCastelCoordY;
        seacher->destX = enemyCastelCoordX;
      }else{
        seacher->destY = gameStage.targetY;
        seacher->destX = gameStage.targetX;
      }
    }

    /*
     * 資源マス破壊ユニットの目的地更新
     */
    void updateVillageBreakerDestination(Unit *breaker){
      Coord coord = searchEnemyOccupiedResourceNode(breaker->y, breaker->x);

      // 相手の占領している資源マスを発見した
      if(coord.y != UNDEFINED && coord.x != UNDEFINED){
        // 目的地が決まっていない場合は新しく設定
        if(breaker->destY == UNDEFINED && breaker->destX == UNDEFINED){
          breaker->destY = coord.y;
          breaker->destX = coord.x;
        }
      }else{
        breaker->destY = gameStage.targetY;
        breaker->destX = gameStage.targetX;
      }
    }

    /*
     * ユニットのモードを決める
     */
    int directUnitMode(Unit *unit){
      Node *node = getNode(unit->y, unit->x);
      assert(unit->y >= 0 && unit->x >= 0 && myCastelCoordY >= 0 && myCastelCoordX >= 0);
      int castelDist = calcManhattanDist(unit->y, unit->x, myCastelCoordY, myCastelCoordX);
      if(unit->role == LEADER){
        updateTroopsLimit(unit);
      }

      switch(unit->type){
        case WORKER:
          if(unit->mode == PICKING || (gameStage.incomeResource < createLimit && !isCreateLimitReach() && pickModeCheck(unit))){
            if(node->resource && node->myUnitCount[VILLAGE] >= 1 && castelDist >= 20 && unit->birthday <= 20){
              return SEARCH;
            }else{
              return PICKING;
            }
          }else if(unit->role == KING){
            return STAY;
          }else if(unit->mode == SPY){
            int dist = calcManhattanDist(unit->y, unit->x, 99, 99);

            if(dist <= 12){
              return SEARCH;
            }else{
              return SPY;
            }
          }else{
            return SEARCH;
          }
          break;
        case KNIGHT:
          switch(unit->role){
            case LEADER:
              if(unit->troopsCount >= unit->troopsLimit){
                if(unit->townId != NULL){
                  unit->townId = NULL;
                  node->troopsId = UNDEFINED;
                }

                return DESTROY;
              }else if(unit->mode == STAY && unit->troopsCount < unit->troopsLimit){
                return STAY;
              }else{
                return DESTROY;
              }
              break;
            case COMBATANT:
              if(unitList[unit->leaderId].mode == DESTROY){
                return DESTROY;
              }else if(unit->mode == SEARCH){
                return SEARCH;
              }else{
                return STAY;
              }
            default:
              return SEARCH;
              break;
          }
          break;
        case FIGHTER:
          switch(unit->role){
            case LEADER:
              if(unit->troopsCount >= unit->troopsLimit || gameStage.incomeResource <= 15){
                if(unit->townId != NULL){
                  unit->townId = NULL;
                  node->troopsId = UNDEFINED;
                }

                return DESTROY;
              }else if(unit->mode == STAY && unit->troopsCount < unit->troopsLimit){
                return STAY;
              }else{
                return DESTROY;
              }
              break;
            case COMBATANT:
              if(unitList[unit->leaderId].mode == DESTROY){
                return DESTROY;
              }else{
                return STAY;
              }
            default:
              return SEARCH;
              break;
          }
          break;
        case ASSASIN:
          switch(unit->role){
            case LEADER:
              if(unit->troopsCount >= unit->troopsLimit || gameStage.incomeResource <= 15){
              //if(unit->troopsCount >= unit->troopsLimit || gameStage.gameSituation == DANGER){
                if(unit->townId != NULL){
                  unit->townId = NULL;
                  node->troopsId = UNDEFINED;
                }

                return DESTROY;
              }else if(unit->mode == STAY && unit->troopsCount < unit->troopsLimit){
                return STAY;
              }else{
                return DESTROY;
              }
              break;
            case COMBATANT:
              if(unitList[unit->leaderId].mode == DESTROY){
                return DESTROY;
              }else{
                return STAY;
              }
              break;
            default:
              return SEARCH;
              break;
          }
          break;
        default:
          return NONE;
      }

      return NONE;
    }

    /*
     * 試合状況の更新を行う
     */
    void updateGameSituation(){
      Node *castel = getNode(myCastelCoordY, myCastelCoordX);
      int aroundCastelEnemyUnitCount  = aroundEnemyUnitCount(myCastelCoordY, myCastelCoordX, 10).totalCount;
      int aroundCastelMyUnitCount     = aroundMyUnitCount(myCastelCoordY, myCastelCoordX, 10).totalCount;

      if(isChokudai() && gameStage.gameSituation == DANGER) return;
      if(isLila() && gameStage.gameSituation == DANGER) return;

      if(enemyCastelCoordY != UNDEFINED && enemyCastelCoordX != UNDEFINED){
        gameStage.gameSituation = ONRUSH;
      }else if(gameStage.gameSituation != WARNING && enemyActiveUnitList.size() == 0){
        gameStage.gameSituation = OPENING;
      }else{
        gameStage.gameSituation = WARNING;
      }

      if(attackCount >= 20 && isChokudai(false)){
        gameStage.gameSituation = DANGER;
      }
      if(attackCount >= 30 && isLila(false)){
        gameStage.gameSituation = DANGER;
      }

      if(castel->myUnitCount[BASE] == 0 || (aroundCastelMyUnitCount < aroundCastelEnemyUnitCount+20)){
        // 敵との距離を測る
        set<int>::iterator it = enemyActiveUnitList.begin();

        while(it != enemyActiveUnitList.end()){
          assert(*it >= 0);
          Unit *enemy = getUnit(*it);

          assert(enemy->y >= 0 && enemy->x >= 0 && myCastelCoordY >= 0 && myCastelCoordX >= 0);
          int dist = calcManhattanDist(enemy->y, enemy->x, myCastelCoordY, myCastelCoordX);
          if(!isChokudai() && !isLila() && enemy->type != WORKER && enemy->type != ASSASIN && dist <= DANGER_LINE){
            gameStage.gameSituation = DANGER;
          }

          it++;
        }
      }
    }

    /*
     * 周りにリーダが居ないか調べる
     */
    bool existLeader(int y, int x){
      set<int>::iterator it = myActiveUnitList.begin();

      while(it != myActiveUnitList.end()){
        assert(*it >= 0);
        Unit *other = getUnit(*it);
        assert(y >= 0 && x >= 0 && other->y >= 0 && other->x >= 0);
        int dist = calcManhattanDist(y, x, other->y, other->x);

        if(dist <= 1 && other->role == LEADER){
          return true;
        }

        it++;
      }

      return false;
    }

    /*
     * 役割を決める
     */
    int directUnitRole(Unit *unit){
      Node *node = getNode(unit->y, unit->x);
      assert(unit->y >= 0 && unit->x >= 0 && myCastelCoordY >= 0 && myCastelCoordX >= 0);
      int castelDist = calcManhattanDist(unit->y, unit->x, myCastelCoordY, myCastelCoordX);

      if(unit->type == ASSASIN || unit->type == FIGHTER || unit->type == KNIGHT){
        // 城の周りに生成された戦闘部隊は守護者
        if(castelDist <= 10){
          return GUARDIAN;
        }else if(unit->mode == SEARCH){
          if(isChokudai() && unit->type == KNIGHT && attackCount > 1){
            attackCount += 1;
          }
          return unit->type;
        }else if(!existLeader(unit->y, unit->x)){
          node->troopsId = unit->id;
          unit->townId = node;
          unit->troopsCount = 1;

          if(attackCount == 0){
            if(isGrun(false)){
              unit->troopsLimit = 40;
            }else if(isLila(false)){
              unit->troopsLimit = 30;
            }else if(isChokudai(false)){
              unit->troopsLimit = 10;
            }else if(currentStageNumber <= 30){
              unit->troopsLimit = 40;
            }else{
              unit->troopsLimit = 40;
            }
          }else{
            unit->troopsLimit = 1;
          }
          attackCount += 1;


          return LEADER;
        }else{
          return COMBATANT;
        }
      // ワーカで試合終盤で城に生成されたユニットは王様とする
      }else if(unit->type == WORKER && unit->y == myCastelCoordY && unit->x == myCastelCoordX && turn > 50){
        return KING;
      // 城の上に立てられた拠点は、作戦本部とする
      }else if(unit->type == BASE && unit->y == myCastelCoordY && unit->x == myCastelCoordX){
        return GHQ;
      }else{
        return unit->type;
      }
    }
    
    /*
     * 役割を更新する
     */
    void updateUnitRole(){
      set<int>::iterator it = myActiveUnitList.begin();

      while(it != myActiveUnitList.end()){
        assert(*it >= 0);
        Unit *unit = getUnit(*it);
        Node *node = getNode(unit->y, unit->x);

        if(unit->type == VILLAGE && node->myUnitCount[WORKER] >= 5){
          unit->role = COLLIERY;
        }
        if(unit->type == KNIGHT && isSilver()){
          unit->role = KNIGHT;
        }

        it++;
      }
    }

    /*
     * 行動の優先順位を決める
     */
    int directUnitMovePriority(Unit *unit){
      assert(unit->y >= 0 && unit->x >= 0);
      return 1000 * movePriority[unit->role] - calcManhattanDist(unit->y, unit->x, 90, 90) - calcNearWallDistance(unit->y, unit->x);
    }

    /*
     * 採取モードに移行するかどうかの確認
     */
    bool pickModeCheck(Unit *unit){
      set<int>::iterator it = resourceNodeList.begin();
      if(unit->mode == SPY) return false;

      while(it != resourceNodeList.end()){
        int y = (*it)/WIDTH;
        int x = (*it)%WIDTH;
        assert(unit->y >= 0 && unit->x >= 0 && y >= 0 && x >= 0);
        int dist = calcManhattanDist(unit->y, unit->x, y, x);
        int rightBottomDist = calcManhattanDist(unit->y, unit->x, 99, 99);
        Node *node = getNode(y,x);

        if(!node->rockon && dist <= 10 && checkMinDist(y, x, dist) && !isDie(unit, y, x) && rightBottomDist >= 100){
          node->rockon = true;
          unit->resourceY = y;
          unit->resourceX = x;
          return true;
        }

        it++;
      }

      return false;
    }

    /*
     * 一番距離が近いかの確認
     *       y: 調査したいノードのy座標
     *       x: 調査したいノードのx座標
     * minDist: 現在の最短(調べたいノードとユニットの現在の距離)
     */
    bool checkMinDist(int y, int x, int minDist){
      set<int>::iterator it = myActiveUnitList.begin();

      while(it != myActiveUnitList.end()){
        Unit *unit = &unitList[(*it)];

        if(unit->movable && unit->mode == SEARCH){
          assert(unit->y >= 0 && unit->x >= 0 && y >= 0 && x >= 0);
          int dist = calcManhattanDist(unit->y, unit->x, y, x);

          // 他に最短距離なユニットがいる場合はfalseを返す
          if(minDist > dist) return false;
          //if(minDist > dist && unit->resourceY == UNDEFINED && unit->resourceX == UNDEFINED) return false;
        }

        it++;
      }

      return true;
    }

    /*
     * 自軍の生存確認
     * ユニットのtimestampが更新されていない場合は前のターンで敵に倒されたので、
     * リストから排除する。
     */
    void myUnitSurvivalCheck(){
      set<int> tempList = myActiveUnitList;
      set<int>::iterator id = tempList.begin();

      while(id != tempList.end()){
        Unit *unit = getUnit(*id);

        if(unit->timestamp != turn){
          removeMyUnit(unit);
        }else{
          updateNodeTimestamp(unit->y, unit->x, unit->eyeRange);
        }

        id++;
      }
    }

    /*
     * 敵軍の生存確認
     */
    void enemyUnitSurvivalCheck(){
      set<int> tempList = enemyActiveUnitList;
      set<int>::iterator it = tempList.begin();

      while(it != tempList.end()){
        Unit *enemy = &unitList[*it];
        Node *node = getNode(enemy->y, enemy->x);

        if(enemy->timestamp != turn && node->timestamp == turn){
          removeEnemyUnit(enemy);
        }else if(enemy->timestamp == turn){
          checkEnemyMark(enemy);
        }

        it++;
      }
    }

    /*
     * 敵の城から攻撃をうけたかどうかの確認
     */
    void enemyCastelAttackCheck(){
      set<int>::iterator it = myActiveUnitList.begin();

      while(it != myActiveUnitList.end()){
        Unit *unit = &unitList[*it];

        if((unit->mode == SEARCH || unit->mode == DESTROY || unit->mode == SPY) && !gameStage.castelAttack && underAttack(unit) && isCastelDamage(unit)){
          unit->castelAttackCount += 1;

          if(!gameStage.castelAttack){
            gameStage.castelAttack = true;
            setCastelPointList(unit->y,unit->x);

            if(gameStage.enemyCastelPointList.size() == 0){
              queue<int> que;
              gameStage.enemyCastelPointList = que;
              gameStage.castelAttack = false;
            }else{
              int id = gameStage.enemyCastelPointList.front(); gameStage.enemyCastelPointList.pop();
              int y = id / WIDTH;
              int x = id % WIDTH;

              hitPointY = unit->y;
              hitPointX = unit->x;
              movePriority[WORKER] = 99;
              fprintf(stderr,"turn = %d, hitPointY = %d, hitPointX = %d\n", turn, hitPointY, hitPointX);

              gameStage.targetY = y;
              gameStage.targetX = x;
            }
          }else{
            assert(false);
          }
        }else{
          unit->castelAttackCount = 0;
        }

        it++;
      }
    }

    /*
     * 敵から攻撃を受けたかどうかの確認
     */
    bool underAttack(Unit *unit){
      if(turn == 175){
        //fprintf(stderr,"turn = %d, id = %d, beforeHp = %d, hp = %d\n", turn, unit->id, unit->beforeHp, unit->hp);
      }
      return unit->beforeHp > unit->hp;
    }

    /*
     * 評価値の計算
     */
    int calcEvaluation(Unit *unit, int operation){
      switch(unit->type){
        case WORKER:
          switch(unit->mode){
            case SEARCH:
              assert(unit->destY >= 0 && unit->destX >= 0 && unit->destY < HEIGHT && unit->destX < WIDTH);
              return calcSeacherEvaluation(unit, operation);
              break;
            case PICKING:
              return calcPickingEvaluation(unit, operation);
              break;
            case SPY:
              return calcSpyEvaluation(unit, operation);
              break;
            case STAY:
              return calcKingEvaluation(unit, operation);
              break;
            default:
              break;
          }
          break;
        case VILLAGE:
          switch(unit->mode){
            case NONE:
              return calcVillageEvaluation(unit, operation);
              break;
          }
          break;
        case CASTEL:
          switch(unit->mode){
            case NONE:
              return calcCastelEvaluation(unit, operation);
              break;
          }
        case BASE:
          switch(unit->role){
            case BASE:
              return calcBaseEvaluation(unit, operation);
              break;
            case GHQ:
              return calcGhqEvaluation(unit, operation);
              break;
            default:
              return calcBaseEvaluation(unit, operation);
              break;
          }
          break;
        case KNIGHT:
          if(unit->role == LEADER){
            return calcLeaderEvaluation(unit, operation);
          }else if(unit->role == GUARDIAN){
            return calcGuardianEvaluation(unit, operation);
          }else if(unit->mode == SEARCH){
            assert(unit->destY >= 0 && unit->destX >= 0 && unit->destY < HEIGHT && unit->destX < WIDTH);
            return calcSeacherEvaluation(unit, operation);
          }else if(unit->role == COMBATANT){
            return calcCombatEvaluation(unit, operation);
          }else{
            assert(unit->destY >= 0 && unit->destX >= 0 && unit->destY < HEIGHT && unit->destX < WIDTH);
            return calcSeacherEvaluation(unit, operation);
          }
          break;
        case FIGHTER:
          if(unit->role == LEADER){
            return calcLeaderEvaluation(unit, operation);
          }else if(unit->role == GUARDIAN){
            return calcGuardianEvaluation(unit, operation);
          }else if(unit->role == COMBATANT){
            return calcCombatEvaluation(unit, operation);
          }else if(unit->mode == RESOURCE_BREAK){
            return calcVillageBreakerEvaluation(unit, operation);
          }else{
            assert(unit->destY >= 0 && unit->destX >= 0 && unit->destY < HEIGHT && unit->destX < WIDTH);
            return calcSeacherEvaluation(unit, operation);
          }
          break;
        case ASSASIN:
          if(unit->role == LEADER){
            return calcLeaderEvaluation(unit, operation);
          }else if(unit->role == GUARDIAN){
            return calcGuardianEvaluation(unit, operation);
          }else if(unit->role == COMBATANT){
            return calcCombatEvaluation(unit, operation);
          }else if(unit->mode == RESOURCE_BREAK){
            return calcVillageBreakerEvaluation(unit, operation);
          }else if(unit->mode == SEARCH){
            assert(unit->destY >= 0 && unit->destX >= 0 && unit->destY < HEIGHT && unit->destX < WIDTH);
            return calcSeacherEvaluation(unit, operation);
          }else{
            assert(unit->destY >= 0 && unit->destX >= 0 && unit->destY < HEIGHT && unit->destX < WIDTH);
            return calcSeacherEvaluation(unit, operation);
          }
          break;
        default:
          break;
      }

      return 0;
    }

    /*
     * 拠点を作成するかどうかの判定
     */
    bool canBuildBase(Unit* unit){
      int rightDownDist = calcManhattanDist(unit->y, unit->x, 99, 99);
      int wallDist = calcNearWallDistance(unit->y, unit->x);

      if(gameStage.gameSituation == DANGER) return false;
      if(isSilver()) return false;
      if(gameStage.gameSituation != DANGER && gameStage.baseCount != 0) return false;

      if(isGrun()){
        if(!isSafePoint(unit->y, unit->x, 8, 1)) return false;
        if(wallDist > 15 && myResourceCount <= 1800) return false;
        if(rightDownDist > 40) return false;
      }else{
        if(!isSafePoint(unit->y, unit->x, 2)) return false;
        if(wallDist < 15 && myResourceCount <= 1800) return false;
        if(isLila()){
          if(rightDownDist > 60 && myResourceCount <= 1800) return false;
        }else{
          if(rightDownDist > 60 && myResourceCount <= 1800) return false;
        }
      }

      if(isEnemyCastelDetected()){
        int enemyCastelDist = calcManhattanDist(unit->y, unit->x, enemyCastelCoordY, enemyCastelCoordX);
        if(enemyCastelDist <= 11) return false;
      }

      return true;
    }
    
    /*
     * 突撃が成功するかどうかを確認、相手の城周りの敵の数、自分のユニットの数を考慮して考える
     */
    bool canAssult(Unit *unit, int ypos, int xpos, int range = 2){
      assert(isEnemyCastelDetected());
      int enemyCastelDist = calcManhattanDist(ypos, xpos, enemyCastelCoordY, enemyCastelCoordX);
      UnitCount myUnitCount = aroundMyUnitCount(unit->y, unit->x, 3);
      UnitCount aroundEnemyCastelMyUnit = aroundMyUnitCount(enemyCastelCoordY, enemyCastelCoordX, 9);
      UnitCount enemyCount = aroundEnemyUnitCount(enemyCastelCoordY, enemyCastelCoordX, 1);

      myUnitCount.fighterCount = max(0, myUnitCount.fighterCount - 2 * enemyCount.assasinCount);

      if(!isLila() && myUnitCount.assasinCount + myUnitCount.fighterCount > min(10, enemyCastelDist) + (enemyCount.assasinCount + enemyCount.fighterCount/2 + enemyCount.knightCount/3)){
        return true;
      }else if(isLila() && !isEnemyCastelSpy()){
        return false;
      }else if(enemyCount.assasinCount + enemyCount.fighterCount + enemyCount.knightCount/2 <= 12 + aroundEnemyCastelMyUnit.totalCount){
        return true;
      }else{
        return false;
      }
    }

    /*
     * 拠点周りの敵ユニットを見て有利なユニットの生成を行う
     */
    int checkAdvantageousUnit(int ypos, int xpos, int range = 10){
      int knightCount   = 0;
      int fighterCount  = 0;
      int assasinCount  = 0;

      map<int, bool> checkList;
      queue<cell> que;
      que.push(cell(Coord(ypos, xpos), 0));

      while(!que.empty()){
        cell c = que.front(); que.pop();

        Coord coord = c.first;
        int dist = c.second;

        if(checkList[coord.y*WIDTH+coord.x] || dist > range) continue;
        checkList[coord.y*WIDTH+coord.x] = true;

        Node *node = getNode(coord.y, coord.x);
        knightCount   += node->enemyUnitCount[KNIGHT];
        fighterCount  += node->enemyUnitCount[FIGHTER];
        assasinCount  += node->enemyUnitCount[ASSASIN];

        for(int i = 1; i < 5; i++){
          int ny = coord.y + dy[i];
          int nx = coord.x + dx[i];

          if(!isWall(ny,nx)) que.push(cell(Coord(ny,nx), dist+1));
        }
      }

      int maxUnitCount = max(max(knightCount/4, fighterCount), assasinCount);

      if(maxUnitCount == assasinCount){
        return CREATE_ASSASIN;
      }else if(maxUnitCount == fighterCount){
        return CREATE_ASSASIN;
      }else{
        return CREATE_FIGHTER;
      }
    }

    /*
     * 探索組の評価値
     */
    int calcSeacherEvaluation(Unit *unit, int operation){
      assert(unit->destY >= 0 && unit->destX >= 0 && unit->destY < HEIGHT && unit->destX < WIDTH);
      int topLeftDist = calcManhattanDist(unit->y, unit->x, 0, 0);
      int destDist = calcManhattanDist(unit->y, unit->x, unit->destY, unit->destX);
      Node *node = getNode(unit->y, unit->x);
      int stamp = node->stamp;
      
      if(operation == NO_MOVE){
        return MIN_VALUE;
      }else if(operation == CREATE_BASE && canBuildBase(unit)){
        return 10000000;
      }else{
        if(hitPointY == UNDEFINED && isDie(unit, unit->y, unit->x)){
          return myResourceCount - 10000;
        }else if(turn <= 60){
          if(gameStage.openedNodeCount == 0){
            return 100 * myResourceCount - 20 * destDist - 3 * stamp - node->cost + 10 * aroundMyUnitDist(unit);
          }else{
            return 100 * myResourceCount + 2 * gameStage.openedNodeCount - 10 * destDist - 3 * stamp - node->cost + 10 * aroundMyUnitDist(unit);
          }
        }else{
          if(unit->type != WORKER){
            if(isEnemyCastelDetected()){
              destDist = abs(calcManhattanDist(unit->y, unit->x, enemyCastelCoordY, enemyCastelCoordX));
              return 100 * myResourceCount - 10 * destDist - max(0, calcReceivedCombatDamage(unit)-1000)/10;
            }else{
              return 100 * myResourceCount + 2 * gameStage.openedNodeCount - 10 * destDist - 5 * stamp - node->cost - (node->receiveDamage[unit->type]/10) + 5 * aroundMyUnitDist(unit);
            }
          }else{
            if(isEnemyCastelDetected() && operation != CREATE_VILLAGE && operation != CREATE_BASE){
              assert(enemyCastelCoordY >= 0 && enemyCastelCoordX >= 0);
              Node *enemyCastel = getNode(enemyCastelCoordY, enemyCastelCoordX);
              int aroundCastelEnemyCount = aroundEnemyUnitCount(enemyCastelCoordY, enemyCastelCoordX, 1).totalCount;
              int dist = calcManhattanDist(unit->y, unit->x, enemyCastelCoordY, enemyCastelCoordX);
              int diff = (aroundCastelEnemyCount+min(12, dist/2)+5 <= 20 || dist <= 10)? 0 : 12;
              destDist = abs(diff-calcManhattanDist(unit->y, unit->x, enemyCastelCoordY, enemyCastelCoordX));

              if(enemyCastel->enemyUnitCount[BASE] == 0){
                return 100 * myResourceCount - 10 * destDist;
              }else{
                return 100 * myResourceCount + 5 * gameStage.openedNodeCount - 10 * destDist - 5 * stamp - node->cost - max(0, calcReceivedCombatDamage(unit)-300)/10;
              }
            // 敵の攻撃を受けた地点であれば即村を建てて、敵の城の位置を把握する
            }else if(operation == CREATE_VILLAGE && unit->y == hitPointY && unit->x == hitPointX && node->myUnitCount[VILLAGE] <= 1){
              return MAX_VALUE;
            // 敵の城が監視されていない場合は再度城の建設を行う(対Lila対策)
            }else if(operation == CREATE_VILLAGE && isEnemyCastelDetected() && isLila() && !isEnemyCastelSpy() && calcManhattanDist(unit->y, unit->x, enemyCastelCoordY, enemyCastelCoordX) <= 10){
              return MAX_VALUE;
            // 基本的に村や拠点は作成しない
            }else if(operation == CREATE_VILLAGE || operation == CREATE_BASE){
              return MIN_VALUE;
            // 前にいた場所に戻るのはNG
            }else if(unit->y == unit->beforeY && unit->x == unit->beforeX){
              return MIN_VALUE;
            }else{
              int dangerPoint = dangerPointList[unit->y][unit->x];
              //int dangerPoint = (calcDangerPoint(unit->y, unit->x, 3) >= 3)? 100 : 0;

              if(isGrun()){
                return 50 * myResourceCount + 10 * gameStage.openedNodeCount - 5 * destDist - 2 * stamp - node->cost - 10 * (node->receiveDamage[unit->type])/100 + 10 * topLeftDist - 2 * dangerPoint;
              }else{
                return 50 * myResourceCount + 4 * gameStage.openedNodeCount - 10 * destDist - 2 * stamp - node->cost - (node->receiveDamage[unit->type])/10 + 10 * topLeftDist - 2 * dangerPoint;
              }
            }
          }
        }
      }
    }

    int calcKingEvaluation(Unit *king, int operation){
      Node *node = getNode(myCastelCoordY, myCastelCoordX);

      if(operation == CREATE_BASE && node->myUnitCount[BASE] <= 1 && gameStage.gameSituation == DANGER){
        return 100;
      }else if(operation == CREATE_BASE && node->myUnitCount[BASE] <= 2 && myResourceCount >= 800 && gameStage.gameSituation == DANGER){
        return 100;
      }else if((enemyAI != SILVER && enemyAI != CHOKUDAI && enemyAI != LILA) && operation == NO_MOVE){
        return 200;
      }else if(operation == NO_MOVE){
        return 10;
      }else{
        return 0;
      }
    }

    /*
     * SPY状態の評価値
     */
    int calcSpyEvaluation(Unit *unit, int operation){
      Node *node = getNode(unit->y, unit->x);
      int dist = calcManhattanDist(unit->y, unit->x, 99, 99);
      int diffY   = abs(unit->y - 99);
      int diffX   = abs(unit->x - 99);
      int diffAll = abs(diffY - diffX);
      int rightUpDist = calcManhattanDist(unit->y, unit->x, 0, 99);
      int leftDownDist = calcManhattanDist(unit->y, unit->x, 99, 0);

      if(operation == CREATE_BASE && canBuildBase(unit)){
        return 10000000;
      }else if(operation == CREATE_VILLAGE && unit->y == hitPointY && unit->x == hitPointX && node->myUnitCount[VILLAGE] <= 1){
        return MAX_VALUE;
      }else if(operation == NO_MOVE){
        return MIN_VALUE;
      }else if(operation == CREATE_VILLAGE || operation == CREATE_BASE){
        return MIN_VALUE;
      }else if(unit->y == unit->beforeY && unit->x == unit->beforeX){
        return MIN_VALUE;
      }else{
        if(myCastelCoordY < myCastelCoordX){
          if(unit->id == 1){
            return -2 * dist - rightUpDist - 2 * dangerPointList[unit->y][unit->x];
          }else{
            return -2 * dist - leftDownDist - 2 * dangerPointList[unit->y][unit->x];
          }
        }else{
          if(unit->id == 1){
            return -2 * dist - leftDownDist - 2 * dangerPointList[unit->y][unit->x];
          }else{
            return -2 * dist - rightUpDist - 2 * dangerPointList[unit->y][unit->x];
          }
        }
      }
    }

    /*
     * PICKING状態での評価値
     * 資源マスにいない状態では資源マスを目指すように
     */
    int calcPickingEvaluation(Unit *unit, int operation){
      Node *node = getNode(unit->y, unit->x);
      int dist = calcManhattanDist(unit->y, unit->x, 50, 50);

      // 資源マスの上にいる場合
      if(node->resource && unit->resourceY == unit->y && unit->resourceX == unit->x){
        //fprintf(stderr,"Base Count = %d\n", node->myUnitCount[BASE]);
        if(gameStage.gameSituation == WARNING && operation == CREATE_BASE && node->myUnitCount[BASE] == 1){
          return -10000;
        }else if(operation == CREATE_BASE && dist <= 80 && myResourceCount >= 1000 && node->myUnitCount[BASE] == 1){
          return -1000;
        }else if(operation == CREATE_VILLAGE && gameStage.createVillageCount == 0 && gameStage.gameSituation != DANGER && isSafePoint(unit->y, unit->x, 4) && gameStage.incomeResource < createLimit && node->myUnitCount[VILLAGE] == 1){
          return 1000;
        }else if(operation == CREATE_VILLAGE || operation == CREATE_BASE){
          return MIN_VALUE;
        }else if(node->myUnitCount[WORKER] == 5 && operation == NO_MOVE){
          return MAX_VALUE;
        }else{
          return myResourceCount + 10 * node->myUnitCount[WORKER];
        }
      }else{
        if(!isEnemyCastelDetected() && isDie(unit, unit->y, unit->x)){
          unit->mode = SEARCH;
          unit->resourceY = UNDEFINED;
          unit->resourceX = UNDEFINED;
          unit->destY = gameStage.targetY;
          unit->destX = gameStage.targetX;
          return -10000;
        }else{
          int diffY   = abs(unit->y - unit->resourceY);
          int diffX   = abs(unit->x - unit->resourceX);
          int diffAll = abs(diffY - diffX);
          assert(unit->resourceY >= 0 && unit->resourceX >= 0 && unit->resourceY < HEIGHT && unit->resourceX < WIDTH);
          return -4 * calcManhattanDist(unit->y, unit->x, unit->resourceY, unit->resourceX) - 2 * diffAll;
        }
      }
    }

    /*
     * 村の行動評価値
     */
    int calcVillageEvaluation(Unit *village, int operation){
      assert(village->y >= 0 && village->x >= 0 && village->y < HEIGHT && village->x < WIDTH);
      int wallDist = calcNearWallDistance(village->y, village->x);
      int centerDist = calcManhattanDist(village->y, village->x, 50, 50);
      int income = gameStage.incomeResource;
      Node *node = getNode(village->y, village->x);

      if(operation == CREATE_WORKER && isSafePoint(village->y, village->x, 8) && node->resource && node->myUnitCount[WORKER] <= 5 && village->createWorkerCount <= 5 && income < createLimit){
        return 100;
      }else if(operation != CREATE_WORKER && (gameStage.gameSituation == DANGER || gameStage.gameSituation == WARNING) && village->createWorkerCount >= 5){
        return 1000;
      }else if(operation == CREATE_WORKER && isSafePoint(village->y, village->x, 8) && wallDist > 10 && node->resource && centerDist <= 70 && village->createWorkerCount <= 5 && isResourceFull()){
        return 110;
      }else if(operation == CREATE_WORKER && isSafePoint(village->y, village->x, 8) && isGrun() && node->resource && centerDist <= 70 && village->createWorkerCount <= 5 && isResourceFull()){
        return 110;
      }else if(operation == CREATE_WORKER && isSilver() && isDefended() && myResourceCount >= 100){
        if(isEnemyCastelDetected()){
          int enemyCastelDist = calcManhattanDist(village->y, village->x, enemyCastelCoordY, enemyCastelCoordX);
          return (enemyCastelDist <= 20)? 1000 : -100;
        }else{
          return -10;
        }
      }else if(operation != CREATE_WORKER){
        return 10;
      }else{
        return 0;
      }
    }

    /*
     * 城の評価値
     */
    int calcCastelEvaluation(Unit *castel, int operation){
      Node *node = getNode(castel->y, castel->x);

      // 序盤でどれだけワーカーの数を増やすか
      if(operation == CREATE_WORKER && turn <= workerLimit){
        return 100;
      }else if(operation == CREATE_WORKER && gameStage.gameSituation == DANGER && node->myUnitCount[WORKER] == 1){
        return 100;
      }else{
        return 0;
      }
    }

    /*
     * 拠点の評価値
     */
    int calcBaseEvaluation(Unit *base, int operation){
      if(myResourceCount <= 100 && operation != NO_MOVE && !isEnemyCastelDetected()) return -100;
      if(myResourceCount <= 100 && operation != NO_MOVE && isEnemyCastelDetected() && isLila() && !isEnemyCastelSpy()) return -100;
      UnitCount myUnitCount = aroundMyUnitCount(base->y, base->x, 10);
      int bestUnit = checkAdvantageousUnit(base->y, base->x);

      if(isEnemyCastelDetected()){
        UnitCount aroundEnemyCastelMyUnitCount = aroundMyUnitCount(enemyCastelCoordY, enemyCastelCoordX, 10);

        if(operation == CREATE_ASSASIN){
          return 100;
        }else if(operation == bestUnit && !isLila() && currentStageNumber <= 50){
          return 110;
        }else if(operation == CREATE_FIGHTER && myUnitCount.assasinCount >= 30){
          return (isGrun())? 110 : 90;
        }else if(operation == CREATE_FIGHTER && (!isLila() && attackCount > 1)){
          return 20;
        }else if(operation == CREATE_FIGHTER && myResourceCount >= 100 && isChokudai()){
          return 140;
        }else if(operation == CREATE_KNIGHT && (isSilver() || isChokudai())){
          return 130;
        }else if((enemyAI == SILVER || enemyAI == CHOKUDAI || enemyAI == LILA) && gameStage.gameSituation == DANGER && operation == NO_MOVE){
          return 200;
        }else{
          return 0;
        }
      }else{
        if(operation == CREATE_ASSASIN){
          return 100;
        }else if(operation == CREATE_FIGHTER &&  myUnitCount.assasinCount >= 30){
          return 150;
        }else if(operation == CREATE_FIGHTER && (!isLila() && attackCount > 1)){
          return 20;
        }else if(operation == CREATE_FIGHTER && myResourceCount >= 100 && isChokudai()){
          return 140;
        }else if(operation == CREATE_KNIGHT && (isSilver() || isChokudai())){
          return 130;
        }else if((enemyAI == SILVER || enemyAI == CHOKUDAI || enemyAI == LILA) && gameStage.gameSituation == DANGER && operation == NO_MOVE){
          return 200;
        }else{
          return 0;
        }
      }
    }

    /*
     * 作戦指令本部の評価値
     */
    int calcGhqEvaluation(Unit *ghq, int operation){
      if(myResourceCount <= 100 && operation != NO_MOVE && !isEnemyCastelDetected()) return -100;

      int myUnitCount = aroundMyUnitCount(ghq->y, ghq->x, 10).totalCount;
      int enemyUnitCount = aroundEnemyUnitCount(ghq->y, ghq->x, 10).totalCount;
      int bestUnit = checkAdvantageousUnit(ghq->y, ghq->x);
      Node *node = getNode(ghq->y, ghq->x);

      if(gameStage.gameSituation == DANGER && (myUnitCount < enemyUnitCount+50 || myResourceCount >= 1000) && operation == bestUnit){
        return 100;
      }else if(node->myUnitCount[BASE] == 2 && operation == CREATE_KNIGHT && myResourceCount >= 600){
        return 50;
      }else if(gameStage.gameSituation != DANGER && operation == NO_MOVE){
        return 10;
      }else{
        return 0;
      }
    }

    /*
     * 周りの味方の数を数える
     */
    UnitCount aroundMyUnitCount(int ypos, int xpos, int range){
      UnitCount myUnitCount;

      map<int, bool> checkList;
      queue<cell> que;
      que.push(cell(Coord(ypos, xpos), 0));

      while(!que.empty()){
        cell c = que.front(); que.pop();

        Coord coord = c.first;
        int dist = c.second;

        if(checkList[coord.y*WIDTH+coord.x] || dist > range) continue;
        checkList[coord.y*WIDTH+coord.x] = true;

        Node *node = getNode(coord.y, coord.x);
        myUnitCount.knightCount += node->myUnitCount[KNIGHT];
        myUnitCount.fighterCount += node->myUnitCount[FIGHTER];
        myUnitCount.assasinCount += node->myUnitCount[ASSASIN];
        myUnitCount.totalCount += node->myUnitTotalCount;

        for(int i = 1; i < 5; i++){
          int ny = coord.y + dy[i];
          int nx = coord.x + dx[i];

          if(!isWall(ny,nx)) que.push(cell(Coord(ny,nx), dist+1));
        }
      }

      return myUnitCount;
    }

    /*
     * 村破壊モードの評価
     */
    int calcVillageBreakerEvaluation(Unit *breaker, int operation){
      Node *node = getNode(breaker->y, breaker->x);
      assert(breaker->y >= 0 && breaker->x >= 0 && breaker->destY >= 0 && breaker->destX >= 0);
      int destDist = calcManhattanDist(breaker->y, breaker->x, breaker->destY, breaker->destX);

      return (100 - destDist);
    }

    /*
     * 守護者時の行動パターン
     */
    int calcGuardianEvaluation(Unit *unit, int operation){
      Node *node = getNode(unit->y, unit->x);
      Node *castel = getNode(myCastelCoordY, myCastelCoordX);
      assert(gameStage.targetY >= 0 && gameStage.targetX >= 0 && gameStage.targetY < HEIGHT && gameStage.targetX < WIDTH);
      int destDist = calcManhattanDist(unit->y, unit->x, gameStage.targetY, gameStage.targetX);
      int castelDist = calcManhattanDist(unit->y, unit->x, myCastelCoordY, myCastelCoordX);
      int penalty = (castel->myUnitTotalCount > 10)? MIN_VALUE : MAX_VALUE;
      int limit = 10;

      if(castel->myUnitCount[unit->type] <= limit){
        return -100 * calcManhattanDist(unit->y, unit->x, myCastelCoordY, myCastelCoordX) + penalty;
      }else{
        return -100 * abs(1-calcManhattanDist(unit->y, unit->x, myCastelCoordY, myCastelCoordX)) - 10 * node->myUnitCount[unit->type] + penalty;
      }

      /*
      if(node->myUnitTotalCount > 10){
        return MIN_VALUE;
      }else if(castelDist > 10 || unit->mode == DESTROY){
        return (10000 - destDist);
      }else if(node->myUnitTotalCount > 10){
        return 100000 - castelDist;
      }else{
        return node->attackDamage[unit->type] - node->receiveDamage[unit->type]/10 - castelDist;
      }
      */
    }

    /*
     * リーダ時の行動パターン
     */
    int calcLeaderEvaluation(Unit *unit, int operation){
      Node *node = getNode(unit->y, unit->x);
      int unitCount = aroundMyUnitCount(unit->y, unit->x, 1).totalCount;
      int stamp = gameStage.field[unit->y][unit->x].stamp;
      assert(gameStage.targetY >= 0 && gameStage.targetX >= 0 && gameStage.targetY < HEIGHT && gameStage.targetX < WIDTH);
      int targetDist = calcManhattanDist(unit->y, unit->x, gameStage.targetY, gameStage.targetX);
      int killCount = (unit->troopsCount > 10)? canKillEnemyCount(unit->y, unit->x, unit->attackRange) : 0;
      //int bestDirect = (unit->troopsLimit > 10)? getNextDirection(unit->y, unit->x, unit->destY, unit->destX) : UNDEFINED;
      int bestDirect = UNDEFINED;

      switch(unit->mode){
        case STAY:
          if(operation != NO_MOVE){
            return -10000;
          }else{
            return 0;
          }
          break;
        case DESTROY:
          if(isEnemyCastelDetected()){
            assert(enemyCastelUnitId >= 0);
            Node *node = getNode(enemyCastelCoordY, enemyCastelCoordX);
            Unit *enemyCastel = getUnit(enemyCastelUnitId);
            int enemyCastelDist = calcManhattanDist(unit->y, unit->x, enemyCastelCoordY, enemyCastelCoordX);
            int diff = (enemyCastel->hp < 50000 || enemyCastelDist <= 11 || canAssult(unit, enemyCastelCoordY, enemyCastelCoordX) || node->enemyUnitCount[BASE] == 0)? 0 : 12;
            Coord bestCoord(enemyCastelCoordY, enemyCastelCoordX);

            if(isLila() && !canAssult(unit, enemyCastelCoordY, enemyCastelCoordX) && enemyCastelDist > 11){
              bestCoord = stanbyPositionCoord(unit);
              enemyCastelDist = calcManhattanDist(unit->y, unit->x, bestCoord.y, bestCoord.x);
              diff = 0;
            }

            int diffY = abs(unit->y - bestCoord.y);
            int diffX = abs(unit->x - bestCoord.x);
            int diffAll = abs(diffY - diffX);
            int wallDist = calcNearWallDistance(unit->y, unit->x);

            if(isLila() && !isEnemyCastelSpy() && enemyCastelDist > 11){
              diff = 12;
            }

            if(bestDirect == operation){
              return MAX_VALUE;
            }

            if(isGrun()){
              return -20 * abs(diff-enemyCastelDist) - 2 * wallDist - node->receiveDamage[unit->type]/10;
            }else if(isSilver()){
              if(enemyCastelDist > 10){
                return -20 * abs(diff-enemyCastelDist) - 2 * wallDist - node->receiveDamage[unit->type];
              }else{
                return -20 * abs(diff-enemyCastelDist) - 5 * diffAll - 2 * wallDist - node->receiveDamage[unit->type]/10;
              }
            }else{
              return -20 * abs(diff-enemyCastelDist) - 5 * diffAll - node->receiveDamage[unit->type]/10;
            }
          }else if(!gameStage.castelAttack){
            int diffY = abs(unit->y - gameStage.targetY);
            int diffX = abs(unit->x - gameStage.targetX);
            int diffAll = abs(diffY - diffX);
            return -5 * targetDist + calcManhattanDist(unit->y, unit->x, 0, 0) - stamp + 2 * diffAll - node->receiveDamage[unit->type]/10;
          }else{
            int diffY = abs(unit->y - gameStage.targetY);
            int diffX = abs(unit->x - gameStage.targetX);
            int diffAll = abs(diffY - diffX);
            int wallDist = calcNearWallDistance(unit->y, unit->x);

            if(isGrun()){
              return -4 * targetDist + 5 * killCount - 2 * wallDist - node->receiveDamage[unit->type]/10;
            }else{
              return -4 * targetDist - 2 * diffAll - node->receiveDamage[unit->type]/10;
            }
          }
          break;
        default:
          if(gameStage.gameSituation == ONRUSH){
            return -1 * calcManhattanDist(unit->y, unit->x, enemyCastelCoordY, enemyCastelCoordX);
          }else if(gameStage.field[unit->y][unit->x].myUnitCount[unit->type] <= 4){
            return -1 * calcManhattanDist(unit->y, unit->x, unit->destY, unit->destX);
          }else if(gameStage.gameSituation == ONRUSH){
            return -1 * calcManhattanDist(unit->y, unit->x, enemyCastelCoordY, enemyCastelCoordX);
          }else{
            return -1 * calcManhattanDist(unit->y, unit->x, 0, 0);
          }
          break;
      }

      return -100;
    }

    /*
     * 戦闘員の行動評価関数
     */
    int calcCombatEvaluation(Unit *unit, int operation){
      assert(unit->leaderId >= 0);
      Unit *leader = getUnit(unit->leaderId);
      Node *node = getNode(unit->y, unit->x);
      int limit = (leader->y == enemyCastelCoordY && leader->x == enemyCastelCoordX)? 10 : 10;
      int leaderDist = calcManhattanDist(unit->y, unit->x, leader->y, leader->x);
      int penalty = (node->myUnitTotalCount > 10)? MIN_VALUE : MAX_VALUE;
      int value = (leader->currentOperation == operation)? 100 : 0;

      switch(unit->mode){
        case STAY:
          assert(leader->y >= 0 && leader->x >= 0);
          if(gameStage.field[leader->y][leader->x].myUnitCount[unit->type] <= limit){
            return -100 * calcManhattanDist(unit->y, unit->x, leader->y, leader->x) + penalty;
          }else{
            return -100 * abs(1-calcManhattanDist(unit->y, unit->x, leader->y, leader->x)) - 10 * node->myUnitCount[unit->type] + penalty;
          }
          break;
        case DESTROY:
          return value;
          if(isEnemyCastelDetected()){
            int enemyCastelDist = calcManhattanDist(unit->y, unit->x, enemyCastelCoordY, enemyCastelCoordX);

            if(enemyCastelDist <= 2){
              return (100 - enemyCastelDist) + penalty;
            }else if(leaderDist > 2){
              return (100 - leaderDist);
            }else{
              return value;
            }
          }else{
            return value;
          }
          break;
        case SEARCH:
          int enemyCastelDist = calcManhattanDist(unit->y, unit->x, enemyCastelCoordY, enemyCastelCoordX);
          if(enemyCastelDist <= 2){
            return (100 - enemyCastelDist) + penalty;
          }else if(leaderDist > 2){
            return (100 - leaderDist);
          }else{
            return value;
          }
          break;
      }

      return -1000;
    }

    /*
     * 自軍のユニットが生きているかどうかの確認
     */
    bool isAlive(int unitId){
      return (myActiveUnitList.find(unitId) != myActiveUnitList.end());
    }
    
    /*
     * 自分の攻撃範囲内にいる敵で倒せる数を数える
     */
    int canKillEnemyCount(int ypos, int xpos, int attackRange){
      int killCount = 0;

      set<int>::iterator it = enemyActiveUnitList.begin();

      while(it != enemyActiveUnitList.end()){
        Unit *enemy = getUnit(*it);

        assert(enemy->y >= 0 && enemy->x >= 0 && enemy->y < HEIGHT && enemy->x < WIDTH);
        int dist = calcManhattanDist(ypos, xpos, enemy->y, enemy->x);
        if(dist <= attackRange){
          killCount += (int)isKilled(enemy);
        }

        it++;
      }

      return killCount;
    }

    /*
     * 周囲の敵の数を数える
     */
    UnitCount aroundEnemyUnitCount(int ypos, int xpos, int range){
      UnitCount enemyCount;

      map<int, bool> checkList;
      queue<cell> que;
      que.push(cell(Coord(ypos, xpos), 0));

      while(!que.empty()){
        cell c = que.front(); que.pop();

        Coord coord = c.first;
        int dist = c.second;

        if(checkList[coord.y*WIDTH+coord.x] || dist > range) continue;
        checkList[coord.y*WIDTH+coord.x] = true;

        Node *node = getNode(coord.y, coord.x);
        enemyCount.totalCount += node->enemyUnitTotalCount;
        enemyCount.knightCount += node->enemyUnitCount[KNIGHT];
        enemyCount.fighterCount += node->enemyUnitCount[FIGHTER];
        enemyCount.assasinCount += node->enemyUnitCount[ASSASIN];

        for(int i = 1; i < 5; i++){
          int ny = coord.y + dy[i];
          int nx = coord.x + dx[i];

          if(!isWall(ny,nx)) que.push(cell(Coord(ny,nx), dist+1));
        }
      }


      return enemyCount;
    }

    /*
     * 自軍ユニットとの距離
     */
    int aroundMyUnitDist(Unit *unit){
      int minDist = 9999;
      int secondMinDist = 9999;
      priority_queue< Coord, vector<Coord>, greater<Coord>  > que;

      set<int>::iterator it = myActiveUnitList.begin();

      while(it != myActiveUnitList.end()){
        Unit *other = getUnit(*it);

        if(unit->id != other->id && other->movable){
          assert(other->y >= 0 && other->x >= 0);
          int dist = calcManhattanDist(unit->y, unit->x, other->y, other->x);

          if(minDist > dist){
            secondMinDist = minDist;
            minDist = dist;
          }else if(secondMinDist > dist){
            secondMinDist = dist;
          }
        }

        it++;
      }

      return min(minDist, secondMinDist);
    }

    /*
     * 敵の攻撃を受ける可能性があるマスにチェックを付ける
     */
    void checkEnemyMark(Unit *enemy){

      map<int, bool> checkList;
      queue<cell> que;
      que.push(cell(Coord(enemy->y, enemy->x), 0));

      while(!que.empty()){
        cell c = que.front(); que.pop();

        Coord coord = c.first;
        int dist = c.second;

        if(checkList[coord.y*WIDTH+coord.x] || dist > enemy->attackRange) continue;
        checkList[coord.y*WIDTH+coord.x] = true;

        Node *node = getNode(coord.y, coord.x);
        node->enemyAttackCount[enemy->type] += 1;

        for(int i = 1; i < 5; i++){
          int ny = coord.y + dy[i];
          int nx = coord.x + dx[i];

          if(!isWall(ny,nx)) que.push(cell(Coord(ny,nx), dist+1));
        }
      }
    }

    /*
     * 敵から受けるかもしれないダメージを計算する
     */
    int calcReceivedCombatDamage(Unit *unit){
      int damage = 0;
      Node *node = getNode(unit->y, unit->x);

      for(int enemyType = 0; enemyType < UNIT_MAX; enemyType++){
        damage += DAMAGE_TABLE[enemyType][unit->type] * node->enemyAttackCount[enemyType];
      }

      return damage;
    }

    /*
     * ノードを取得する
     */
    inline Node* getNode(int y, int x){
      assert(y >= 0 && x >= 0 && y < HEIGHT && x < WIDTH);
      return &gameStage.field[y][x];
    }

    /*
     * ユニット情報を取得する
     */
    inline Unit* getUnit(int unitId){
      assert(unitId >= 0 && unitId <= 20000);
      return &unitList[unitId];
    }

    /*
     * 探索時の基本的なコストを付ける
     * - 壁から3マスは移動しなくても探索可能
     */
    void checkCost(){
      for(int y = 0; y < HEIGHT; y++){
        for(int x = 0; x < WIDTH; x++){
          Node *node = getNode(y,x);
          int dist = calcNearWallDistance(y,x);

          if(dist <= 3){
            node->cost = (4-dist) * 6;
          }

          if(y < 4){
            node->cost += 10000;
          }
          if(x < 4){
            node->cost += 10000;
          }
          if(y > HEIGHT-5){
            node->cost += 10000;
          }
          if(x > WIDTH-5){
            node->cost += 10000;
          }
        }
      }
    }

    /*
     * 敵の城が無いことをチェックする
     */
    void checkNoEnemyCastel(int y, int x){
      map<int, bool> checkList;
      queue<cell> que;
      que.push(cell(Coord(y, x), 0));

      while(!que.empty()){
        cell c = que.front(); que.pop();

        Coord coord = c.first;
        int dist = c.second;

        if(checkList[coord.y*WIDTH+coord.x] || dist > 10) continue;
        checkList[coord.y*WIDTH+coord.x] = true;

        Node *node = getNode(coord.y, coord.x);
        node->enemyCastel = false;

        for(int i = 1; i < 5; i++){
          int ny = coord.y + dy[i];
          int nx = coord.x + dx[i];

          if(!isWall(ny,nx)) que.push(cell(Coord(ny,nx), dist+1));
        }
      }
    }

    /*
     * 近くの壁までの距離を測る
     */
    int calcNearWallDistance(int y, int x){
      int up = y;
      int down = (HEIGHT-1) - y;
      int left = x;
      int right = (WIDTH-1) - x;

      return min(min(up,down), min(left,right));
    }

    /*
     * 安産地帯かどうかを調べる
     */
    bool isSafePoint(int ypos, int xpos, int eyeRange, int safeLine = 0){
      int enemyCount = 0;

      assert(ypos >= 0 && xpos >= 0);

      set<int>::iterator it = enemyActiveUnitList.begin();

      while(it != enemyActiveUnitList.end()){
        assert(*it >= 0);
        Unit *enemy = getUnit(*it);
        int dist = calcManhattanDist(ypos, xpos, enemy->y, enemy->x);

        if(dist <= eyeRange && enemy->type != WORKER && enemy->type != VILLAGE){
          enemyCount += 1;
        }

        it++;
      }

      return enemyCount <= safeLine;
    }

    /*
     * 危険度を計算する
     */
    int calcDangerPoint(int ypos, int xpos, int eyeRange, bool spy = false){
      int dangerPoint = 0;

      assert(ypos >= 0 && xpos >= 0);
      set<int>::iterator it = enemyActiveUnitList.begin();

      while(it != enemyActiveUnitList.end()){
        assert(*it >= 0);
        Unit *enemy = getUnit(*it);
        int dist = calcManhattanDist(ypos, xpos, enemy->y, enemy->x);

        if(dist <= eyeRange){
          if(enemy->type == ASSASIN){
            dangerPoint += 3;
          }else if(enemy->type == FIGHTER){
            dangerPoint += 2;
          }else if(enemy->type == KNIGHT){
            dangerPoint += 1;
          }else if(spy){
            dangerPoint += 1;
          }
        }

        it++;
      }

      return dangerPoint;
    }

    /*
     * 危険度を更新
     */
    void updateDangerPoint(){
      memset(dangerPointList, 0, sizeof(dangerPointList));

      set<int>::iterator id = enemyActiveUnitList.begin();

      while(id != enemyActiveUnitList.end()){
        Unit *enemy = getUnit(*id);

        if(enemy->timestamp == turn){
          if(enemy->type == ASSASIN){
            addDangerPoint(enemy->y, enemy->x, 3, 3); 
          }else if(enemy->type == FIGHTER){
            addDangerPoint(enemy->y, enemy->x, 3, 2); 
          }else if(enemy->type == FIGHTER){
            addDangerPoint(enemy->y, enemy->x, 3, 1); 
          }else if(enemy->type == WORKER){
            addDangerPoint(enemy->y, enemy->x, 2, 1); 
          }
        }

        id++;
      }
    }

    /*
     * 危険度を加算する
     */
    void addDangerPoint(int ypos, int xpos, int range, int point){
      map<int, bool> checkList;
      queue<cell> que;
      que.push(cell(Coord(ypos, xpos), 0));

      while(!que.empty()){
        cell c = que.front(); que.pop(); 
        Coord coord = c.first;
        int dist = c.second;

        if(checkList[coord.y*WIDTH+coord.x] || dist > range) continue;
        checkList[coord.y*WIDTH+coord.x] = true;

        dangerPointList[coord.y][coord.x] += point;

        for(int i = 1; i < 5; i++){
          int ny = coord.y + dy[i];
          int nx = coord.x + dx[i];
          if(!isWall(ny,nx)) que.push(cell(Coord(ny, nx), dist+1));
        }
      }
    }

    /*
     * 敵の城を発見したかどうか
     */
    bool isEnemyCastelDetected(){
      return (enemyCastelCoordY != UNDEFINED && enemyCastelCoordX != UNDEFINED);
    }

    /*
     * 自分の城に拠点があるかどうか
     */
    bool isDefended(){
      Node *node = getNode(myCastelCoordY, myCastelCoordX);

      return node->myUnitCount[BASE] > 0;
    }

    /*
     * 敵の城に拠点があるかどうか
     */
    bool isEnemyDefended(){
      assert(enemyCastelCoordY >= 0 && enemyCastelCoordX >= 0 && enemyCastelCoordY < HEIGHT && enemyCastelCoordX < WIDTH);
      Node *node = getNode(enemyCastelCoordY, enemyCastelCoordX);

      if(node->enemyUnitCount[BASE] > 0){
        maybeAI[CHOKUDAI] = false;
        maybeAI[COLUN] = false;
        maybeAI[ROSA] = false;
      }

      return node->enemyUnitCount[BASE] > 0;
    }

    /*
     * 敵の城からのダメージを受けたかどうかの判定
     */
    bool isCastelDamage(Unit *unit){
      int currentHp = unit->beforeHp;

      set<int>::iterator it = enemyActiveUnitList.begin();
      if(unit->timestamp != turn) return false;

      while(it != enemyActiveUnitList.end()){
        Unit *enemy = getUnit(*it);
        int dist = calcManhattanDist(unit->y, unit->x, enemy->y, enemy->x);

        if(dist <= unitAttackRange[enemy->type]){
          int k = countMyUnit(enemy->y, enemy->x, unitAttackRange[enemy->type]);
          if(enemy->type == VILLAGE || enemy->type == BASE || enemy->timestamp != turn) return false;
          if(k > 0){
            currentHp -= DAMAGE_TABLE[enemy->type][unit->type] / k;
          }
        }else if(dist == 3 && enemy->timestamp != turn){
          return false;
        }

        it++;
      }

      return (currentHp != unit->hp);
    }

    /*
     * 死ぬかどうかの確認
     */
    bool isDie(Unit *unit, int ypos, int xpos){
      int currentHp = unit->hp;
      set<int>::iterator it = enemyActiveUnitList.begin();

      assert(ypos >= 0 && xpos >= 0);

      while(it != enemyActiveUnitList.end()){
        Unit *enemy = &unitList[*it];
        int dist = calcManhattanDist(ypos, xpos, enemy->y, enemy->x);

        if(dist <= unitAttackRange[enemy->type]){
          int k = countMyUnit(enemy->y, enemy->x, unitAttackRange[enemy->type]);
          if(k > 0){
            currentHp -= DAMAGE_TABLE[enemy->type][unit->type] / k;
          }
        }

        it++;
      }

      return currentHp < unit->hp * 0.7;
    }

    /*
     * 範囲内の敵が倒せるかどうかの確認
     */
    bool isKilled(Unit *enemy){
      int currentHp = enemy->hp;
      int k;
      assert(enemy->y >= 0 && enemy->x >= 0);

      set<int>::iterator it = myActiveUnitList.begin();
      map<int, int> enemyCountCash;

      while(it != myActiveUnitList.end()){
        Unit *unit = &unitList[*it];
        int dist = calcManhattanDist(enemy->y, enemy->x, unit->y, unit->x);

        if(dist <= unitAttackRange[unit->type]){
          if(enemyCountCash[unit->y*WIDTH+unit->x] > 0){
            k = enemyCountCash[unit->y*WIDTH+unit->x];
          }else{
            k = countEnemyUnit(unit->y, unit->x, unitAttackRange[unit->type]);
            enemyCountCash[unit->y*WIDTH+unit->x] = k;
          }
          if(k > 0){
            currentHp -= DAMAGE_TABLE[unit->type][enemy->type] / k;
          }
        }

        it++;
      }

      return currentHp <= 0;
      //return currentHp <= enemy->hp * 0.9;
    }

    /*
     * 敵の城の候補地を選出する(初めてダメージを受けたマスから範囲10のマスのどこか)
     * 1回の試合で1回しか呼ばれない
     */
    void setCastelPointList(int ypos, int xpos){
      assert(ypos >= 0 && xpos >= 0);

      for(int y = 0; y < HEIGHT; y++){
        for(int x = 0; x < WIDTH; x++){
          int dist = calcManhattanDist(ypos, xpos, y, x);
          Node *node = getNode(y,x);

          if(dist == 10 && node->enemyCastel && isCastelPoint(y,x)){
            gameStage.enemyCastelPointList.push(y*WIDTH+x);
          }
        }
      }
    }

    /*
     * そこが敵の城かどうかを判定する
     */
    bool isCastelPoint(int y, int x){
      map<int, bool> checkList;
      queue<cell> que;
      que.push(cell(Coord(y, x), 0));

      if(calcManhattanDist(y, x, 99, 99) > 40) return false;

      while(!que.empty()){
        cell c = que.front(); que.pop();

        Coord coord = c.first;
        int dist = c.second;

        if(checkList[coord.y*WIDTH+coord.x] || dist > 4) continue;
        checkList[coord.y*WIDTH+coord.x] = true;

        Node *node = &gameStage.field[coord.y][coord.x];

        if(node->searched) return false;

        for(int i = 1; i < 5; i++){
          int ny = coord.y + dy[i];
          int nx = coord.x + dx[i];

          if(!isWall(ny,nx)) que.push(cell(Coord(ny,nx), dist+1));
        }
      }

      return true;
    }

    /*
     * 範囲内にいる自軍の数を数える
     */
    int countMyUnit(int y, int x, int range){
      map<int, bool> checkList;
      queue<cell> que;
      int unitCount = 0;
      que.push(cell(Coord(y, x), 0));

      while(!que.empty()){
        cell c = que.front(); que.pop();

        Coord coord = c.first;
        int dist = c.second;

        if(checkList[coord.y*WIDTH+coord.x] || dist > range) continue;
        checkList[coord.y*WIDTH+coord.x] = true;

        Node *node = getNode(coord.y, coord.x);

        unitCount += min(10, node->myUnitTotalCount);

        for(int i = 1; i < 5; i++){
          int ny = coord.y + dy[i];
          int nx = coord.x + dx[i];

          if(!isWall(ny,nx)) que.push(cell(Coord(ny,nx),dist+1));
        }
      }

      return unitCount;
    }

    /*
     * 範囲内にいる敵軍の数を数える
     */
    int countEnemyUnit(int y, int x, int range){
      map<int, bool> checkList;
      queue<cell> que;
      int unitCount = 0;
      que.push(cell(Coord(y, x), 0));

      while(!que.empty()){
        cell c = que.front(); que.pop();

        Coord coord = c.first;
        int dist = c.second;

        if(checkList[coord.y*WIDTH+coord.x] || dist > range) continue;
        checkList[coord.y*WIDTH+coord.x] = true;

        Node *node = getNode(coord.y, coord.x);

        unitCount += min(10, node->enemyUnitTotalCount);

        for(int i = 1; i < 5; i++){
          int ny = coord.y + dy[i];
          int nx = coord.x + dx[i];

          if(!isWall(ny,nx)) que.push(cell(Coord(ny,nx),dist+1));
        }
      }

      return unitCount;
    }

    /*
     * マークを付ける
     */
    void checkMark(int ypos, int xpos){
      assert(ypos >= 0 && xpos >= 0 && ypos < WIDTH && xpos < HEIGHT);
      int cost = 4;
      map<int, bool> checkList;
      queue<cell> que;
      que.push(cell(Coord(ypos, xpos), cost));

      while(!que.empty()){
        cell c = que.front(); que.pop(); 
        Coord coord = c.first;
        int cost = c.second;

        if(checkList[coord.y*WIDTH+coord.x] || cost < 0) continue;
        checkList[coord.y*WIDTH+coord.x] = true;

        gameStage.field[coord.y][coord.x].markCount += 1;

        for(int i = 1; i < 5; i++){
          int ny = coord.y + dy[i];
          int nx = coord.x + dx[i];
          if(!isWall(ny,nx)) que.push(cell(Coord(ny, nx), cost-1));
        }
      }
    }

    /*
     * マークを外す
     */
    void uncheckMark(int ypos, int xpos){
      int cost = 8;
      map<int, bool> checkList;
      queue<cell> que;
      que.push(cell(Coord(ypos, xpos), cost));

      while(!que.empty()){
        cell c = que.front(); que.pop(); 
        Coord coord = c.first;
        int cost = c.second;

        if(checkList[coord.y*WIDTH+coord.x] || cost < 0) continue;
        checkList[coord.y*WIDTH+coord.x] = true;

        gameStage.field[coord.y][coord.x].markCount -= 1;

        for(int i = 1; i < 5; i++){
          int ny = coord.y + dy[i];
          int nx = coord.x + dx[i];
          if(!isWall(ny,nx)) que.push(cell(Coord(ny, nx), cost-1));
        }
      }
    }

    /*
     * 足跡を付ける
     */
    void checkStamp(int ypos, int xpos, int eyeRange){
      assert(ypos >= 0 && xpos >= 0 && ypos < HEIGHT && xpos < WIDTH);
      for(int y = max(0, ypos-eyeRange); y <= min(HEIGHT-1, ypos+eyeRange); y++){
        int diff = 2*abs(ypos-y)/2;

        for(int x = max(0, xpos-eyeRange+diff); x <= min(WIDTH-1, xpos+eyeRange-diff); x++){
          if(isWall(y,x)) continue;

          gameStage.field[y][x].stamp += 1;
        }
      }
    }

    /*
     * 現在確保出来ている視界の数を調べる
     */
    int checkVisibleCount(){
      int visibleNodeCount = 0;

      for(int y = 0; y < HEIGHT; y++){
        for(int x = 0; x < WIDTH; x++){
          if(gameStage.field[y][x].seenCount > 0){
            visibleNodeCount += 1;
          }
        }
      }

      return visibleNodeCount;
    }

    /*
     * fieldの初期化を行う
     */
    void clearField(){
      for(int y = 0; y < HEIGHT; y++){
        for(int x = 0; x < WIDTH; x++){
          Node *node = getNode(y,x);

          node->seenCount = 0;
          if(turn % 10 == 0){
            node->markCount = 0;
          }
          node->seenMembers.clear();
          node->myUnitTotalCount = 0;
          node->myUnits.clear();
          node->enemyUnitTotalCount = 0;
          node->enemyUnits.clear();
          node->opened = false;
          node->myK = 0;
          node->enemyK = 0;

          if(turn % 1 == 0){
            node->stamp = 0;
          }

          memset(node->myUnitCount, 0, sizeof(node->myUnitCount));
          memset(node->enemyUnitCount, 0, sizeof(node->enemyUnitCount));
          memset(node->enemyAttackCount, 0, sizeof(node->enemyAttackCount));
          memset(node->enemyCountWithinAttackRange, 0, sizeof(node->enemyCountWithinAttackRange));
          memset(node->attackDamage, 0, sizeof(node->attackDamage));
          memset(node->receiveDamage, 0, sizeof(node->receiveDamage));
        }
      }
    }

    /*
     * ゲームの実行
     */
    void run(){
      init();

      // 残り時間(ms)が取得出来なくなるまで回し続ける
      while(cin >> remainingTime){
        //fprintf(stderr, "Remaing time is %dms\n", remainingTime);

        // フィールドのクリア
        clearField();

        // 各ターンで行う処理(主に入力の処理)
        eachTurnProc();

        // コストを付ける
        if(turn == 0){
          checkCost();
        }

        // 敵の城から攻撃を受けたかどうかのチェック
        enemyCastelAttackCheck();

        // 自軍の生存確認
        myUnitSurvivalCheck();

        // 敵軍の生存確認
        enemyUnitSurvivalCheck();

        // 資源マス情報の更新
        updateResourceNodeData();

        // 戦闘情報の更新
        updateBattleData();

        // 危険度の更新
        updateDangerPoint();

        // 自軍の役割の更新を行う
        updateUnitRole();

        // 収入の更新
        updateIncomeResource();

        // 対戦AIの考察
        checkEnemyAI();

        // 試合状況更新
        updateGameSituation();

        // 自軍の各ユニットのモード変更を行う
        updateUnitMode();

        // 自軍の各ユニットの目的地の更新を行う
        updateUnitDestination();

        vector<Operation> operationList;
        // 行動フェーズ
        operationList = actionPhase();

        // 最終的な出力
        finalOperation(operationList);
      }
    }

    /*
     * 最終指示(このターンの最終的な行動を出力)
     */
    void finalOperation(vector<Operation> &operationList){
      int size = operationList.size();

      printf("%d\n", size);
      for(int i = 0; i < size; i++){
        Operation ope = operationList[i];
        printf("%d %c\n", ope.unitId, instruction[reverseOperation(ope.operation)]);
      }
    }

    /*
     * 視界をチェックする(探索済みのマスを増やす)
     *   unitId: ユニットID
     *     ypos: y座標
     *     xpos: x座標
     * eyeRange: 視界の広さ
     */
    void checkNode(int unitId, int ypos, int xpos, int eyeRange){
      assert(ypos >= 0 && xpos >= 0 && ypos < HEIGHT && xpos < WIDTH);
      for(int y = max(0, ypos-eyeRange); y <= min(HEIGHT-1, ypos+eyeRange); y++){
        int diff = 2*abs(ypos-y)/2;

        for(int x = max(0, xpos-eyeRange+diff); x <= min(WIDTH-1, xpos+eyeRange-diff); x++){
          if(isWall(y,x)) continue;
          Node *node = getNode(y,x);

          node->seenMembers.insert(unitId);
          node->seenCount += 1;

          if(!node->searched){
            gameStage.searchedNodeCount += 1;
            node->searched = true;

            if(mostDownCoordY < y && mostRightCoordX < x){
              mostDownCoordY  = y;
              mostRightCoordX = x;
            }
          }

          if(!node->opened){
            gameStage.visibleNodeCount += 1;
            node->opened = true;
          }
        }
      }
    }

    /*
     * 視界のアンチェックを行う
     */
    void uncheckNode(int unitId, int ypos, int xpos, int eyeRange){
      assert(ypos >= 0 && xpos >= 0 && ypos < HEIGHT && xpos < WIDTH);

      for(int y = max(0, ypos-eyeRange); y <= min(HEIGHT-1, ypos+eyeRange); y++){
        int diff = 2*abs(ypos-y)/2;

        for(int x = max(0, xpos-eyeRange+diff); x <= min(WIDTH-1, xpos+eyeRange-diff); x++){
          if(isWall(y,x)) continue;
          Node *node = getNode(y,x);

          node->seenMembers.erase(unitId);
        }
      }
    }

    /*
     * 視界をオープンする(仮想的)
     *      ypos: y座標
     *      xpos: x座標
     *  eyeRange: 視界の広さ
     */
    void openNode(int ypos, int xpos, int eyeRange){
      assert(ypos >= 0 && xpos >= 0 && ypos < HEIGHT && xpos < WIDTH);
      for(int y = max(0, ypos-eyeRange); y <= min(HEIGHT-1, ypos+eyeRange); y++){
        int diff = 2*abs(ypos-y)/2;

        for(int x = max(0, xpos-eyeRange+diff); x <= min(WIDTH-1, xpos+eyeRange-diff); x++){
          if(isWall(y,x)) continue;

          Node *node = getNode(y,x);

          node->seenCount += 1;

          gameStage.openedNodeCount += !node->searched;
          gameStage.visibleNodeCount += !node->opened;

          node->opened = true;
        }
      }
    }

    /*
     * 視界をクローズする(openNodeのrollback用)
     *      ypos: y座標
     *      xpos: x座標
     *  eyeRange: 視界の広さ
     */
    void closeNode(int ypos, int xpos, int eyeRange){
      assert(ypos >= 0 && xpos >= 0 && ypos < HEIGHT && xpos < WIDTH);

      for(int y = max(0, ypos-eyeRange); y <= min(HEIGHT-1, ypos+eyeRange); y++){
        int diff = 2*abs(ypos-y)/2;

        for(int x = max(0, xpos-eyeRange+diff); x <= min(WIDTH-1, xpos+eyeRange-diff); x++){
          if(isWall(y,x)) continue;

          Node *node = getNode(y,x);
          node->seenCount -= 1;

          bool opened = (node->seenCount > 0);

          gameStage.openedNodeCount -= !node->searched;
          gameStage.visibleNodeCount -= node->opened ^ opened;
          gameStage.field[y][x].opened = opened;
        }
      }
    }


    /*
     * ユニットが行動を起こす
     * 行動が成功した場合はtrue、失敗した場合は場合はfalseを返す
     *   unit: ユニット
     *   type: 行動の種別
     *  final: 確定した行動の場合はtrue
     */
    bool unitAction(Unit *unit, int operationType, bool final = false){
      if(final){
        unit->moved = true;
      }

      switch(operationType){
        case MOVE_UP:
          if(canMove(unit->y, unit->x, MOVE_UP)){
            closeNode(unit->y, unit->x, unit->eyeRange);

            moveUp(unit);

            if(final){
              checkNode(unit->id, unit->y, unit->x, unit->eyeRange);
            }else{
              openNode(unit->y, unit->x, unit->eyeRange);
            }
          }else{
            return false;
          }
          break;
        case MOVE_DOWN:
          if(canMove(unit->y, unit->x, MOVE_DOWN)){
            closeNode(unit->y, unit->x, unit->eyeRange);

            moveDown(unit);

            if(final){
              checkNode(unit->id, unit->y, unit->x, unit->eyeRange);
            }else{
              openNode(unit->y, unit->x, unit->eyeRange);
            }
          }else{
            return false;
          }
          break;
        case MOVE_LEFT:
          if(canMove(unit->y, unit->x, MOVE_LEFT)){
            closeNode(unit->y, unit->x, unit->eyeRange);

            moveLeft(unit);

            if(final){
              checkNode(unit->id, unit->y, unit->x, unit->eyeRange);
            }else{
              openNode(unit->y, unit->x, unit->eyeRange);
            }
          }else{
            return false;
          }
          break;
        case MOVE_RIGHT:
          if(canMove(unit->y, unit->x, MOVE_RIGHT)){
            closeNode(unit->y, unit->x, unit->eyeRange);

            moveRight(unit);

            if(final){
              checkNode(unit->id, unit->y, unit->x, unit->eyeRange);
            }else{
              openNode(unit->y, unit->x, unit->eyeRange);
            }
          }else{
            return false;
          }
          break;
        case CREATE_WORKER:
          if(canBuild(unit->type, WORKER)){
            createUnit(unit->y, unit->x, WORKER);

            if(final){
              unit->createWorkerCount += 1;
            }
          }else{
            return false;
          }
          break;
        case CREATE_KNIGHT:
          if(canBuild(unit->type, KNIGHT)){
            createUnit(unit->y, unit->x, KNIGHT);

            if(final){
              unit->createKnightCount += 1;
            }
          }else{
            return false;
          }
          break;
        case CREATE_FIGHTER:
          if(canBuild(unit->type, FIGHTER)){
            createUnit(unit->y, unit->x, FIGHTER);

            if(final){
              unit->createFighterCount += 1;
            }
          }else{
            return false;
          }
          break;
        case CREATE_ASSASIN:
          if(canBuild(unit->type, ASSASIN)){
            createUnit(unit->y, unit->x, ASSASIN);

            if(final){
              unit->createAssasinCount += 1;
            }
          }else{
            return false;
          }
          break;
        case CREATE_VILLAGE:
          if(canBuild(unit->type, VILLAGE)){
            createUnit(unit->y, unit->x, VILLAGE);

            if(final){
              gameStage.createVillageCount += 1;
            }
          }else{
            return false;
          }
          break;
        case CREATE_BASE:
          if(canBuild(unit->type, BASE)){
            createUnit(unit->y, unit->x, BASE);

            if(final){
              gameStage.baseCount += 1;
            }
          }else{
            return false;
          }
          break;
        default:
          noMove();
          break;
      }

      return true;
    }

    /*
     * ユニットのアクションの取消を行う
     * unitId: ユニットID
     *   type: アクションの種類
     */
    void rollbackAction(Unit *unit, int type){
      switch(type){
        case MOVE_UP:
          closeNode(unit->y, unit->x, unit->eyeRange);
          moveDown(unit);
          openNode(unit->y, unit->x, unit->eyeRange);
          break;
        case MOVE_DOWN:
          closeNode(unit->y, unit->x, unit->eyeRange);
          moveUp(unit);
          openNode(unit->y, unit->x, unit->eyeRange);
          break;
        case MOVE_LEFT:
          closeNode(unit->y, unit->x, unit->eyeRange);
          moveRight(unit);
          openNode(unit->y, unit->x, unit->eyeRange);
          break;
        case MOVE_RIGHT:
          closeNode(unit->y, unit->x, unit->eyeRange);
          moveLeft(unit);
          openNode(unit->y, unit->x, unit->eyeRange);
          break;
        case CREATE_WORKER:
          deleteUnit(unit->y, unit->x, WORKER);
          break;
        case CREATE_KNIGHT:
          deleteUnit(unit->y, unit->x, KNIGHT);
          break;
        case CREATE_FIGHTER:
          deleteUnit(unit->y, unit->x, FIGHTER);
          break;
        case CREATE_ASSASIN:
          deleteUnit(unit->y, unit->x, ASSASIN);
          break;
        case CREATE_VILLAGE:
          deleteUnit(unit->y, unit->x, VILLAGE);
          break;
        case CREATE_BASE:
          deleteUnit(unit->y, unit->x, BASE);
          break;
        default:
          noMove();
          break;
      }
    }

    /*
     * ユニットの数を移動させる
     *        y: y座標
     *        x: x座標
     *   direct: 動く方向
     * unitType: ユニットの種類
     */
    void moveUnitCount(int y, int x, int direct, int unitType){
      assert(y >= 0 && x >= 0 && y < HEIGHT && x < WIDTH);
      gameStage.field[y][x].myUnitCount[unitType] -= 1;
      gameStage.field[y][x].myUnitTotalCount -= 1;

      gameStage.field[y+dy[direct]][x+dx[direct]].myUnitCount[unitType] += 1;
      gameStage.field[y+dy[direct]][x+dx[direct]].myUnitTotalCount += 1;
    }

    /*
     * 何も行動しない
     */
    void noMove(){
    }

    /*
     * 上に動く
     */
    void moveUp(Unit *unit){
      moveUnitCount(unit->y, unit->x, MOVE_UP, unit->type);
      unit->y -= 1;
    }

    /*
     * 下に動く
     */
    void moveDown(Unit *unit){
      moveUnitCount(unit->y, unit->x, MOVE_DOWN, unit->type);
      unit->y += 1;
    }

    /*
     * 左に動く
     */
    void moveLeft(Unit *unit){
      moveUnitCount(unit->y, unit->x, MOVE_LEFT, unit->type);
      unit->x -= 1;
    }

    /*
     * 右に動く
     */
    void moveRight(Unit *unit){
      moveUnitCount(unit->y, unit->x, MOVE_RIGHT, unit->type);
      unit->x += 1;
    }

    /*
     * 行動フェーズ
     * 自軍に対して各種行動を選択する
     */
    vector<Operation> actionPhase(){
      set<int>::iterator it = myActiveUnitList.begin();
      vector<Operation> operationList;
      priority_queue<MovePriority, vector<MovePriority>, greater<MovePriority> > prique;

      // 各ユニット毎に処理を行う
      while(it != myActiveUnitList.end()){
        MovePriority mp(*it, directUnitMovePriority(&unitList[*it]));
        prique.push(mp);
        it++;
      }

      while(!prique.empty()){
        MovePriority mp = prique.top(); prique.pop();
        assert(mp.unitId >= 0);
        Unit *unit = getUnit(mp.unitId);

        Operation bestOperation = directUnitOperation(unit);

        // 行動なし以外はリストに入れる
        if(bestOperation.operation != NONE){
          operationList.push_back(bestOperation);

          // 前の行動の座標を記録しておく
          unit->beforeY = unit->y;
          unit->beforeX = unit->x;

          // 確定した行動はそのままにする
          unitAction(unit, bestOperation.operation, REAL);


          // 足跡を付ける(足跡が多い場所はなるべく探索しないように)
          if(unit->mode == SEARCH){
            checkStamp(unit->y, unit->x, unit->eyeRange * 2);
          }

          // SEARCHモードのユニットが目的地に到達した場合は、目的地の座標をリセット
          if(unit->mode == SEARCH && (unit->y == unit->destY && unit->x == unit->destX)){
            resetDestPoint(unit);
          }
        }

        // 今回の行動を記録
        unit->currentOperation = bestOperation.operation;
      }

      return operationList;
    }

    /*
     * 目的地のリセットを行う
     */
    void resetDestPoint(Unit *unit){
      assert(unit->destY >= 0 && unit->destX >= 0 && unit->destY < HEIGHT && unit->destX < WIDTH);
      uncheckMark(unit->destY, unit->destX);
      unit->destY = UNDEFINED;
      unit->destX = UNDEFINED;
    }

    /*
     * 命令を決定する
     */
    Operation directUnitOperation(Unit *unit){
      priority_queue<Operation, vector<Operation>, greater<Operation> > que;
      gameStage.searchedNodeCount = 0;
      gameStage.openedNodeCount = 0;

      for(int operation = 0; operation < OPERATION_MAX; operation++){
        if(!OPERATION_LIST[unit->type][operation]) continue;

        // 行動が成功した時だけ評価を行う
        if(unitAction(unit, operation)){
          Operation ope;
          ope.unitId = unit->id;
          ope.operation = operation;
          ope.evaluation = calcEvaluation(unit, operation);

          // 行動を元に戻す
          rollbackAction(unit, operation);

          que.push(ope);
        }
      }

      return que.top();
    }

    /*
     * 渡された座標のマンハッタン距離を計算
     */
    int calcManhattanDist(int y1, int x1, int y2, int x2){
      assert(y1 >= 0 && x1 >= 0 && y1 < HEIGHT && x1 < WIDTH);
      assert(y2 >= 0 && x2 >= 0 && y2 < HEIGHT && x2 < WIDTH);
      return manhattanDist[x1*WIDTH+x2] + manhattanDist[y1*WIDTH+y2];
    }

    /*
     * プレイヤーが2P側なら座標系を逆にする(99, 99)を(0, 0)に
     */
    Coord reverseCoord(int ypos, int xpos){
      // 1P側ならそのまま返す
      if(firstPlayer){
        return Coord(ypos, xpos);
      // 2P側なら逆にしたものを返す
      }else{
        return Coord(reverseCoordTable[ypos],reverseCoordTable[xpos]);
      }
    }

    /*
     * 操作命令を逆にする
     */
    int reverseOperation(int operation){
      if(firstPlayer){
        return operation;
      }else{
        if(operation == MOVE_UP){
          return MOVE_DOWN;
        }else if(operation == MOVE_DOWN){
          return MOVE_UP;
        }else if(operation == MOVE_RIGHT){
          return MOVE_LEFT;
        }else if(operation == MOVE_LEFT){
          return MOVE_RIGHT;
        }else{
          return operation;
        }
      }
    }

    /*
     * 1P側のプレイヤかどうかの確認
     */
    bool isFirstPlayer(){
      assert(myCastelCoordY >= 0 && myCastelCoordX >= 0);
      return (calcManhattanDist(0, 0, myCastelCoordY, myCastelCoordX) <= 40);
    }

    /*
     * 敵の城が監視できているかどうか
     */
    bool isEnemyCastelSpy(){
      assert(isEnemyCastelDetected());
      set<int>::iterator it = myActiveUnitList.begin();
      assert(enemyCastelCoordY >= 0 && enemyCastelCoordX >= 0 && enemyCastelCoordY < HEIGHT && enemyCastelCoordX < WIDTH);

      while(it != myActiveUnitList.end()){
        assert(*it >= 0);
        Unit *unit = getUnit(*it);
        int dist = calcManhattanDist(unit->y, unit->x, enemyCastelCoordY, enemyCastelCoordX);

        if(unit->type == VILLAGE && dist <= 10){
          return true;
        }

        it++;
      }

      return false;
    }

    /*
     * grunかどうかの判定
     */
    bool isGrun(bool check = true){
      set<int>::iterator it = enemyActiveUnitList.begin();

      if(enemyAI == LILA) return false;
      if(enemyAI == GRUN) return true;

      while(it != enemyActiveUnitList.end()){
        assert(*it >= 0);
        Unit *enemy = getUnit(*it);

        if(check && enemy->type == VILLAGE){
          Node *node = getNode(enemy->y, enemy->x);
          int centerDist = calcManhattanDist(enemy->y, enemy->x, 50, 50);

          if(!node->resource && centerDist <= 20){
            enemyAI = GRUN;
            fprintf(stderr,"stage = %d, turn = %d, is Grun!\n", currentStageNumber, turn);
            return true;
          }
        }

        it++;
      }

      return false;
    }

    /*
     * 
     */
    Coord stanbyPositionCoord(Unit *unit){
      assert(enemyCastelCoordY >= 0 &&  enemyCastelCoordX >= 0);

      Coord bestCoord(enemyCastelCoordY - 6, enemyCastelCoordX - 6);
      int minDist = calcManhattanDist(unit->y, unit->x, enemyCastelCoordY - 6, enemyCastelCoordX - 6);
      int dist;

      if(enemyCastelCoordX + 6 < WIDTH){
        dist = calcManhattanDist(unit->y, unit->x, enemyCastelCoordY, enemyCastelCoordX + 6);

        if(minDist > dist){
          minDist = dist;
          bestCoord.y = enemyCastelCoordY;
          bestCoord.x = enemyCastelCoordX + 6;
        }
      }

      if(enemyCastelCoordY + 6 < HEIGHT){
        dist = calcManhattanDist(unit->y, unit->x, enemyCastelCoordY + 6, enemyCastelCoordX);

        if(minDist > dist){
          minDist = dist;
          bestCoord.y = enemyCastelCoordY + 6;
          bestCoord.x = enemyCastelCoordX;
        }
      }

      if(enemyCastelCoordY + 6 < HEIGHT && enemyCastelCoordX + 6 < WIDTH){
        dist = calcManhattanDist(unit->y, unit->x, enemyCastelCoordY + 6, enemyCastelCoordX + 6);

        if(minDist > dist){
          minDist = dist;
          bestCoord.y = enemyCastelCoordY + 6;
          bestCoord.x = enemyCastelCoordX + 6;
        }
      }

      return bestCoord;
    }

    /*
     * Lilaかどうかを判定
     */
    bool isLila(bool check = true){
      set<int>::iterator it = enemyResourceNodeList.begin();

      if(enemyAI == CHOKUDAI) return false;
      if(check && enemyAI == LILA && maybeAI[CHOKUDAI] && isEnemyCastelDetected() && isEnemyCastelSpy() && !isEnemyDefended()){
        fprintf(stderr,"stage = %d, turn = %d, not Lila is Chokudai!\n", currentStageNumber, turn);
        enemyAI = CHOKUDAI;
        return false;
      }else if(enemyAI == LILA){
        return true;
      }

      while(it != enemyResourceNodeList.end()){
        int y = (*it)/WIDTH;
        int x = (*it)%WIDTH;
        Node *node = getNode(y,x);

        if(check && maybeAI[LILA] && node->enemyUnitCount[VILLAGE] == 1){
          createLimit = 40;
          fprintf(stderr,"stage = %d, turn = %d: is Lila!\n", currentStageNumber, turn);
          return true;
        }

        it++;
      }

      return false;
    }

    /*
     * Chokudaiかどうかを判定
     *   - 資源マスの上に村を設置
     *   - 資源マスの上に拠点を設定
     */
    bool isChokudai(bool check = true){
      set<int>::iterator it = resourceNodeList.begin();

      if(maybeAI[CHOKUDAI] && enemyAI == CHOKUDAI) return true;

      int cnt = 0;

      while(it != resourceNodeList.end()){
        int y = (*it)/WIDTH;
        int x = (*it)%WIDTH;
        Node *node = getNode(y,x);

        if(check && isOccupied(y,x)){
          if(node->enemyUnitCount[VILLAGE] == 1 && node->enemyUnitCount[BASE] == 1){
            fprintf(stderr,"stage = %d, turn = %d, is Chokudai!\n", currentStageNumber, turn);
            enemyAI = CHOKUDAI;
            createLimit = 20;
            return true;
          }
        }

        int rightBottomDist = calcManhattanDist(y, x, 99, 99);

        if(check && turn <= 110 && rightBottomDist >= 100 && node->myUnitCount[VILLAGE] == 1 && node->myUnitCount[WORKER] == 5 && node->enemyUnitCount[VILLAGE] > 0){
          cnt++;

          if(cnt > 1){
            fprintf(stderr,"stage = %d, turn = %d, is Chokudai!\n", currentStageNumber, turn);
            enemyAI = CHOKUDAI;
            createLimit = 20;
            return true;
          }
        }

        it++;
      }

      return false;
    }

    /*
     * silverかどうかの判定
     */
    bool isSilver(){
      set<int>::iterator it = enemyResourceNodeList.begin();

      if(enemyAI == LILA) return false;
      if(maybeAI[SILVER] && enemyAI == SILVER) return true;

      while(it != enemyResourceNodeList.end()){
        int y = (*it)/WIDTH;
        int x = (*it)%WIDTH;
        Node *node = getNode(y,x);

        if(node->enemyUnitCount[VILLAGE] == 1){
          maybeAI[SILVER] = false;
          return false;
        }

        it++;
      }

      int diff = (calcManhattanDist(myCastelCoordY, myCastelCoordX, 0, 0) <= 10)? 10 : 0;

      if(turn <= 150 + diff && maybeAI[SILVER] && gameStage.gameSituation == DANGER){
        fprintf(stderr,"stage = %d, turn = %d, is Silver!\n", currentStageNumber, turn);
        enemyAI = SILVER;
        return true;
      }else{
        return false;
      }
    }

    /*
     * 渡された座標が壁かどうかを判定する。
     *   y: y座標
     *   x: x座標
     */
    bool isWall(int y, int x){
      if(y >= 0 && x >= 0 && y < HEIGHT+2 && x < WIDTH+2){
        return walls[y+1][x+1];
      }else{
        return true;
      }
    }

    /*
     * 移動が出来るかどうかのチェックを行う
     *   y: y座標
     *   x: x座標
     */
    bool canMove(int y, int x, int direct){
      int ny = y + dy[direct];
      int nx = x + dx[direct];

      return !isWall(ny,nx);
    }

    /*
     * ユニットの生産が可能かどうか
     *   buildType: 生産したい物
     *   unitTType: ユニットの種類
     */
    bool canBuild(int unitType, int buildType){
      return (OPERATION_LIST[unitType][buildType+5] && unitCost[buildType] <= myResourceCount);
    }

    /*
     * フィールドの表示
     */
    void showField(){
      for(int y = 0; y < HEIGHT; y++){
        for(int x = 0; x < WIDTH; x++){
        }
        fprintf(stderr, "\n");
      }
    }

    /*
     * テスト用のコード(ダミーユニットの作成を行う)
     */
    Unit* createDummyUnit(int unitId, int y, int x, int hp, int unitType){
      addMyUnit(unitId, y, x, hp, unitType);
      return &unitList[unitId];
    }

    /*
     * テスト用のコード(ダミーユニットの作成を行う)
     */
    Unit* createDummyEnemyUnit(int unitId, int y, int x, int hp, int unitType){
      addEnemyUnit(unitId, y, x, hp, unitType);
      return &unitList[unitId];
    }
};

/*
 * ここから下はテストコード
 */
class CodevsTest{
  Codevs cv;

  public:
  void runTest(){
    fprintf(stderr, "TestCase1:\t%s\n", testCase1()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase2:\t%s\n", testCase2()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase3:\t%s\n", testCase3()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase4:\t%s\n", testCase4()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase5:\t%s\n", testCase5()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase6:\t%s\n", testCase6()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase7:\t%s\n", testCase7()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase8:\t%s\n", testCase8()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase9:\t%s\n", testCase9()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase10:\t%s\n", testCase10()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase11:\t%s\n", testCase11()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase12:\t%s\n", testCase12()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase13:\t%s\n", testCase13()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase14:\t%s\n", testCase14()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase15:\t%s\n", testCase15()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase16:\t%s\n", testCase16()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase17:\t%s\n", testCase17()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase18:\t%s\n", testCase18()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase19:\t%s\n", testCase19()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase20:\t%s\n", testCase20()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase21:\t%s\n", testCase21()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase22:\t%s\n", testCase22()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase23:\t%s\n", testCase23()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase24:\t%s\n", testCase24()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase25:\t%s\n", testCase25()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase26:\t%s\n", testCase26()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase27:\t%s\n", testCase27()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase28:\t%s\n", testCase28()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase29:\t%s\n", testCase29()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase30:\t%s\n", testCase30()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase31:\t%s\n", testCase31()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase32:\t%s\n", testCase32()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase33:\t%s\n", testCase33()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase34:\t%s\n", testCase34()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase35:\t%s\n", testCase35()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase36:\t%s\n", testCase36()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase37:\t%s\n", testCase37()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase38:\t%s\n", testCase38()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase39:\t%s\n", testCase39()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase40:\t%s\n", testCase40()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase41:\t%s\n", testCase41()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase42:\t%s\n", testCase42()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase43:\t%s\n", testCase43()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase44:\t%s\n", testCase44()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase45:\t%s\n", testCase45()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase46:\t%s\n", testCase46()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase47:\t%s\n", testCase47()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase48:\t%s\n", testCase48()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase49:\t%s\n", testCase49()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase50:\t%s\n", testCase50()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase51:\t%s\n", testCase51()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase52:\t%s\n", testCase52()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase53:\t%s\n", testCase53()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase54:\t%s\n", testCase54()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase55:\t%s\n", testCase55()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase56:\t%s\n", testCase56()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase57:\t%s\n", testCase57()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase58:\t%s\n", testCase58()? "SUCCESS!" : "FAILED!");
    fprintf(stderr, "TestCase59:\t%s\n", testCase59()? "SUCCESS!" : "FAILED!");
  }

  /*
   * Case1: マンハッタン距離が取得出来ているかどうかの確認
   */
  bool testCase1(){
    if(cv.calcManhattanDist(0,0,1,1) != 2) return false;
    if(cv.calcManhattanDist(0,0,0,0) != 0) return false;
    if(cv.calcManhattanDist(99,99,99,99) != 0) return false;
    if(cv.calcManhattanDist(0,99,99,0) != 198) return false;
    if(cv.calcManhattanDist(3,20,9,19) != 7) return false;
    if(cv.calcManhattanDist(0,0,50,50) != 100) return false;

    return true;
  }

  /*
   * Case2: サンプル入力がしっかりと取れているかどうか
   */
  bool testCase2(){
    if(stageNumber != 0) return false;
    if(turn != 27) return false;
    if(myResourceCount != 29) return false;
    if(myAllUnitCount != 13) return false;
    if(enemyAllUnitCount != 1) return false;
    if(resourceCount != 1) return false;
    if(myCastelCoordY != 7 || myCastelCoordX != 16) return false;

    return true;
  }

  /*
   * Case3: ステージの初期化が成功しているかどうかの確認
   */
  bool testCase3(){
    unitIdCheckList.clear();

    unitIdCheckList[1] = true;
    if(unitIdCheckList.size() != 1) return false;
    cv.stageInitialize();
    if(unitIdCheckList.size() != 0) return false;
    if(myActiveUnitList.size() != 0) return false;
    if(enemyActiveUnitList.size() != 0) return false;
    if(resourceNodeList.size() != 0) return false;
    if(createLimit != 35) return false;
    if(enemyAI != UNDEFINED) return false;
    if(gameStage.enemyCastelPointList.size() != 0) return false;
    if(gameStage.searchedNodeCount != 0) return false;
    if(gameStage.visibleNodeCount != 0) return false;
    if(gameStage.openedNodeCount != 0) return false;
    if(gameStage.castelAttack) return false;

    for(int y = 0; y < HEIGHT; y++){
      for(int x = 0; x < WIDTH; x++){
        Node *node = &gameStage.field[y][x];

        if(node->seenCount != 0) return false;
        if(node->resource) return false;
      } 
    }

    return true;
  }

  /*
   * Case4: 壁判定がちゃんと出来ているかどうか
   */
  bool testCase4(){
    if(!cv.isWall(-1,-1)) return false;
    if(!cv.isWall(-1, 0)) return false;
    if(!cv.isWall(HEIGHT,WIDTH)) return false;
    if(!cv.isWall(HEIGHT-1,WIDTH)) return false;
    if(cv.isWall(10,10)) return false;
    if(cv.isWall(0,0)) return false;
    if(cv.isWall(0,WIDTH-1)) return false;
    if(cv.isWall(HEIGHT-1,0)) return false;
    if(cv.isWall(HEIGHT-1,WIDTH-1)) return false;

    return true;
  }

  /*
   * Case5: 移動判定が出来ているかどうか
   */
  bool testCase5(){
    if(cv.canMove(0,0,MOVE_UP)) return false;
    if(cv.canMove(0,0,MOVE_LEFT)) return false;
    if(!cv.canMove(0,0,MOVE_DOWN)) return false;
    if(!cv.canMove(0,0,MOVE_RIGHT)) return false;
    if(!cv.canMove(0,0,NO_MOVE)) return false;

    return true;
  }

  /*
   * Case6: 「上に移動」が出来ているかどうか
   */
  bool testCase6(){
    cv.stageInitialize();

    Unit *unit = cv.createDummyUnit(0, 10, 10, 2000, WORKER);
    int x = unit->x;
    int y = unit->y;
    gameStage.field[y][x].myUnitCount[WORKER] = 1;

    cv.moveUp(unit);

    if(0 != gameStage.field[y][x].myUnitCount[unit->type]) return false;

    return (x == unit->x && y-1 == unit->y);
  }

  /*
   * Case7: 「下に移動」が出来ているかどうか
   */
  bool testCase7(){
    cv.stageInitialize();
    Unit *unit = cv.createDummyUnit(0, 10, 10, 2000, WORKER);
    int x = unit->x;
    int y = unit->y;
    gameStage.field[y][x].myUnitCount[WORKER] = 1;

    cv.moveDown(unit);

    if(0 != gameStage.field[y][x].myUnitCount[unit->type]) return false;

    return (x == unit->x && y+1 == unit->y);
  }

  /*
   * Case8: 「左に移動」が出来ているかどうか
   */
  bool testCase8(){
    cv.stageInitialize();
    Unit *unit = cv.createDummyUnit(0, 10, 10, 2000, WORKER);
    int x = unit->x;
    int y = unit->y;
    gameStage.field[y][x].myUnitCount[WORKER] = 1;

    cv.moveLeft(unit);

    if(0 != gameStage.field[y][x].myUnitCount[unit->type]) return false;

    return (x-1 == unit->x && y == unit->y);
  }

  /*
   * Case9: 「右に移動」が来ているかどうか
   */
  bool testCase9(){
    cv.stageInitialize();
    Unit *unit = cv.createDummyUnit(0, 10, 10, 2000, WORKER);
    int x = unit->x;
    int y = unit->y;
    gameStage.field[y][x].myUnitCount[WORKER] = 1;

    cv.moveRight(unit);

    if(0 != gameStage.field[y][x].myUnitCount[unit->type]) return false;

    return (x+1 == unit->x && y == unit->y);
  }

  /*
   * Case10: 「生産可否判定」が出来ているかどうか
   */
  bool testCase10(){
    cv.stageInitialize();

    Unit *castel  = cv.createDummyUnit(0, 10, 10, 50000, CASTEL);
    Unit *village = cv.createDummyUnit(1, 11, 11, 20000, VILLAGE);
    Unit *base    = cv.createDummyUnit(2, 12, 12, 20000, BASE);
    Unit *worker  = cv.createDummyUnit(3, 13, 13, 2000, WORKER);

    myResourceCount = 19;
    if(cv.canBuild(castel->type, KNIGHT)) return false;
    if(cv.canBuild(village->type, WORKER)) return false;
    if(cv.canBuild(base->type, KNIGHT)) return false;
    if(cv.canBuild(base->type, FIGHTER)) return false;

    myResourceCount = 20;
    if(!cv.canBuild(base->type, KNIGHT)) return false;
    if(cv.canBuild(worker->type, KNIGHT)) return false;
    if(cv.canBuild(village->type, KNIGHT)) return false;

    myResourceCount = 40;
    if(!cv.canBuild(village->type, WORKER)) return false;
    if(!cv.canBuild(castel->type, WORKER)) return false;
    if(!cv.canBuild(base->type, KNIGHT)) return false;
    if(!cv.canBuild(base->type, FIGHTER)) return false;
    if(cv.canBuild(base->type, ASSASIN)) return false;
    if(cv.canBuild(village->type, FIGHTER)) return false;

    myResourceCount = 60;
    if(!cv.canBuild(base->type, ASSASIN)) return false;
    if(cv.canBuild(castel->type, ASSASIN)) return false;
    if(cv.canBuild(worker->type, VILLAGE)) return false;

    myResourceCount = 100;
    if(!cv.canBuild(worker->type, VILLAGE)) return false;
    if(!cv.canBuild(base->type, KNIGHT)) return false;
    if(!cv.canBuild(base->type, FIGHTER)) return false;
    if(!cv.canBuild(base->type, ASSASIN)) return false;
    if(cv.canBuild(worker->type, BASE)) return false;

    myResourceCount = 500;
    if(!cv.canBuild(worker->type, BASE)) return false;
    if(cv.canBuild(village->type, BASE)) return false;
    if(!cv.canBuild(worker->type, BASE)) return false;
    if(cv.canBuild(castel->type, BASE)) return false;

    return true;
  }

  /*
   * Case11: ユニットが作成できるどうかの確認
   */
  bool testCase11(){
    cv.stageInitialize();

    myResourceCount = 40;
    cv.createUnit(5,5,WORKER);
    if(myResourceCount != 0) return false;
    if(gameStage.field[5][5].myUnitCount[WORKER] != 1) return false;
    if(gameStage.visibleNodeCount != 41) return false;

    myResourceCount = 20;
    cv.createUnit(1,1,KNIGHT);
    if(myResourceCount != 0) return false;
    if(gameStage.field[1][1].myUnitCount[KNIGHT] != 1) return false;
    if(gameStage.visibleNodeCount != 60) return false;

    myResourceCount = 100;
    cv.createUnit(20,20,VILLAGE);
    if(myResourceCount != 0) return false;
    if(gameStage.field[20][20].myUnitCount[VILLAGE] != 1) return false;

    myResourceCount = 500;
    cv.createUnit(50,50,BASE);
    if(myResourceCount != 0) return false;
    if(gameStage.field[50][50].myUnitCount[BASE] != 1) return false;

    return true;
  }

  /*
   * Case12: ノードの作成が出来ているかどうか
   */
  bool testCase12(){
    Node node = cv.createNode();

    if(node.opened) return false;
    if(node.myUnitCount[WORKER] != 0) return false;
    if(node.myUnitCount[FIGHTER] != 0) return false;
    if(node.myUnitCount[BASE] != 0) return false;
    if(node.enemyUnitCount[WORKER] != 0) return false;
    if(node.enemyUnitCount[FIGHTER] != 0) return false;
    if(node.enemyUnitCount[BASE] != 0) return false;
    if(node.enemyCountWithinAttackRange[WORKER] != 0) return false;
    if(node.seenMembers.size() != 0) return false;
    if(node.myUnitTotalCount != 0) return false;
    if(node.enemyUnitTotalCount != 0) return false;
    if(node.rockCount != 0) return false;
    if(node.timestamp != 0) return false;
    if(node.seenCount != 0) return false;
    if(node.myK != 0) return false;
    if(node.enemyK != 0) return false;
    if(node.cost != 0) return false;
    if(node.stamp != 0) return false;
    if(node.markCount != 0) return false;
    if(node.resource) return false;
    if(node.rockon) return false;
    if(node.searched) return false;

    return true;
  }

  /*
   * Case13: ユニットの追加が出来ているかどうかの確認
   */
  bool testCase13(){
    cv.stageInitialize();

    int unitId = 100;
    cv.addMyUnit(unitId, 10, 10, 1980, WORKER);

    Unit *unit = &unitList[unitId];
    if(unit->type != WORKER) return false;
    if(unit->hp != 1980) return false;
    if(unit->mode != SEARCH) return false;
    if(unit->destY != UNDEFINED) return false;
    if(unit->destX != UNDEFINED) return false;
    if(unit->createWorkerCount != 0) return false;
    if(!unit->movable) return false;
    if(!unitIdCheckList[unitId]) return false;
    if(gameStage.searchedNodeCount != 41) return false;
    if(gameStage.field[10][10].seenMembers.size() != 1) return false;

    unitId = 101;
    cv.addMyUnit(unitId, 50, 50, 20000, VILLAGE);
    if(unitList[unitId].type != VILLAGE) return false;
    if(unitList[unitId].hp != 20000) return false;
    if(unitList[unitId].movable) return false;
    if(gameStage.searchedNodeCount != 262) return false;

    unitId = 102;
    cv.addMyUnit(unitId, 30, 30, 20000, BASE);
    if(unitList[unitId].type != BASE) return false;
    if(unitList[unitId].hp != 20000) return false;
    if(unitList[unitId].movable) return false;

    //if(myActiveUnitList.size() != 3) return false;


    return true;
  }

  /*
   * Case14: ユニットの生存確認が出来ているかどうかの確認
   */
  bool testCase14(){
    int unitId = 100;
    cv.stageInitialize();

    cv.addMyUnit(unitId, 10, 10, 1980, WORKER);
    if(gameStage.visibleNodeCount != 41) return false;
    if(gameStage.openedNodeCount != 0) return false;

    unitId = 101;
    cv.addMyUnit(unitId, 20, 20, 1980, WORKER);
    unitList[unitId].timestamp = -1;
    if(gameStage.visibleNodeCount != 82) return false;
    if(gameStage.openedNodeCount != 0) return false;

    cv.myUnitSurvivalCheck();

    if(myActiveUnitList.size() != 1) return false;
    if(myActiveUnitList.find(100) == myActiveUnitList.end()) return false;
    if(myActiveUnitList.find(101) != myActiveUnitList.end()) return false;

    return true;
  }

  /*
   * Case15: ユニットの削除が出来ているかどうかの確認
   */
  bool testCase15(){
    cv.stageInitialize();

    myResourceCount = 80;
    cv.createUnit(1,1,WORKER);
    cv.createUnit(5,5,WORKER);

    cv.deleteUnit(1,1,WORKER);
    if(myResourceCount != 40) return false;
    if(gameStage.visibleNodeCount != 41) return false;
    if(gameStage.field[1][1].myUnitCount[WORKER] != 0) return false;

    cv.deleteUnit(5,5,WORKER);
    if(myResourceCount != 80) return false;
    if(gameStage.visibleNodeCount != 0) return false;
    if(gameStage.field[5][5].myUnitCount[WORKER] != 0) return false;

    return true;
  }

  /*
   * Case16: ユニットが取れるアクションについて制限が取れている
   */
  bool testCase16(){
    cv.stageInitialize();

    int unitId = 100;
    cv.addMyUnit(unitId, 0, 0, 1980, WORKER);

    Unit *unit = &unitList[unitId];
    myResourceCount = COST_MAX;

    if(cv.unitAction(unit, MOVE_UP)) return false;
    if(!cv.unitAction(unit, MOVE_DOWN)) return false;
    if(cv.unitAction(unit, MOVE_LEFT)) return false;
    if(!cv.unitAction(unit, MOVE_RIGHT)) return false;
    if(cv.unitAction(unit, CREATE_WORKER)) return false;
    if(cv.unitAction(unit, CREATE_KNIGHT)) return false;
    if(cv.unitAction(unit, CREATE_FIGHTER)) return false;
    if(cv.unitAction(unit, CREATE_ASSASIN)) return false;
    if(!cv.unitAction(unit, CREATE_VILLAGE)) return false;
    if(!cv.unitAction(unit, CREATE_BASE)) return false;

    return true;
  }

  /*
   * Case17: ロールバックが出来ているかどうか
   */
  bool testCase17(){
    cv.stageInitialize();

    int unitId = 100;
    cv.addMyUnit(unitId, 10, 10, 1980, WORKER);

    Unit *unit = &unitList[unitId];
    myResourceCount = COST_MAX;

    if(gameStage.searchedNodeCount != 41) return false;
    if(gameStage.visibleNodeCount != 41) return false;

    cv.unitAction(unit, MOVE_UP);
    if(gameStage.visibleNodeCount != 41) return false;

    cv.rollbackAction(unit, MOVE_UP);
    if(gameStage.visibleNodeCount != 41) return false;

    return true;
  }

  /*
   * Case18: ユニットの更新が出来ているかどうか
   */
  bool testCase18(){
    cv.stageInitialize();

    int unitId = 100;
    cv.addMyUnit(unitId, 10, 10, 1980, WORKER);

    if(gameStage.field[10][10].seenMembers.size() != 1) return false;
    if(gameStage.field[10][10].myUnitCount[WORKER] != 1) return false;

    cv.updateMyUnitStatus(unitId, 10, 10, 1980);

    if(gameStage.field[10][10].seenMembers.size() != 1) return false;

    return true;
  }

  /*
   * Case19: ユニットの移動の際に視界の広さが取得出来ているかどうか
   */
  bool testCase19(){
    cv.stageInitialize();

    int unitId = 100;
    cv.addMyUnit(unitId, 10, 10, 1980, WORKER);

    unitId = 101;
    cv.addMyUnit(unitId, 10, 10, 1980, WORKER);

    unitId = 102;
    cv.addMyUnit(unitId, 10, 10, 1980, WORKER);

    if(gameStage.visibleNodeCount != 41) return false;

    cv.unitAction(&unitList[100], MOVE_UP);
    if(gameStage.visibleNodeCount != 50) return false;

    cv.unitAction(&unitList[101], MOVE_DOWN);
    if(gameStage.visibleNodeCount != 59) return false;

    cv.rollbackAction(&unitList[100], MOVE_UP);
    cv.rollbackAction(&unitList[101], MOVE_DOWN);

    if(gameStage.visibleNodeCount != 41) return false;

    return true;
  }

  /*
   * Case20: 確保出来ている視界の数が取得できているかどうか
   */
  bool testCase20(){
    cv.stageInitialize();

    int unitId = 100;
    cv.addMyUnit(unitId, 10, 10, 1980, WORKER);

    if(cv.checkVisibleCount() != 41) return false;

    cv.unitAction(&unitList[unitId], MOVE_UP);
    if(gameStage.visibleNodeCount != 41) return false;

    cv.unitAction(&unitList[unitId], MOVE_DOWN);
    if(gameStage.visibleNodeCount != 41) return false;

    return true;
  }

  /*
   * Case21: 調査予定のマスの数が取得出来ているかどうか
   */
  bool testCase21(){
    cv.stageInitialize();

    int unitId = 100;
    cv.addMyUnit(unitId, 10, 10, 1980, WORKER);

    if(gameStage.searchedNodeCount != 41) return false;
    if(gameStage.visibleNodeCount != 41) return false;
    if(gameStage.openedNodeCount != 0) return false;

    cv.unitAction(&unitList[unitId], MOVE_DOWN);
    if(gameStage.visibleNodeCount != 41) return false;
    if(gameStage.searchedNodeCount != 41) return false;
    if(gameStage.openedNodeCount != 9) return false;

    cv.rollbackAction(&unitList[unitId], MOVE_DOWN);
    if(gameStage.searchedNodeCount != 41) return false;
    if(gameStage.visibleNodeCount != 41) return false;
    if(gameStage.openedNodeCount != 0) return false;

    cv.unitAction(&unitList[unitId], MOVE_DOWN, REAL);

    unitId = 101;
    cv.addMyUnit(unitId,  10, 10, 1980, WORKER);

    cv.unitAction(&unitList[unitId], MOVE_LEFT);
    if(gameStage.visibleNodeCount != 50) return false;
    if(gameStage.openedNodeCount != 5) return false;

    return true;
  }

  /*
   * Case22: 同じノードに何体生産しても値が変化しない
   */
  bool testCase22(){
    cv.stageInitialize();

    int unitId = 100;
    myResourceCount = COST_MAX;
    cv.addMyUnit(unitId, 10, 10, 2000, VILLAGE);

    unitId = 101;
    cv.addMyUnit(unitId, 10, 10, 2000, WORKER);

    if(gameStage.searchedNodeCount != 221) return false;
    if(gameStage.visibleNodeCount != 221) return false;
    if(gameStage.openedNodeCount != 0) return false;

    cv.unitAction(&unitList[unitId], MOVE_RIGHT);
    cv.rollbackAction(&unitList[unitId], MOVE_RIGHT);

    if(gameStage.searchedNodeCount != 221) return false;
    if(gameStage.visibleNodeCount != 221) return false;
    if(gameStage.openedNodeCount != 0) return false;

    return true;
  }

  /*
   * Case23: 採取モードに移行出来ているかどうかの確認
   */
  bool testCase23(){
    cv.stageInitialize();

    myResourceCount = COST_MAX;

    Unit *unitA = cv.createDummyUnit(100, 10, 10, 2000, WORKER);
    Unit *unitB = cv.createDummyUnit(101, 11, 11, 2000, WORKER);
    if(unitA->mode != SEARCH) return false;

    cv.addResourceNode(unitA->y+3, unitA->x+3);
    if(cv.directUnitMode(unitA) == PICKING) return false;
    if(cv.directUnitMode(unitB) != PICKING) return false;

    return true;
  }

  /*
   * Case24: 資源マスの追加ができているかどうか
   */
  bool testCase24(){
    cv.stageInitialize();

    if(resourceNodeList.size() != 0) return false;
    cv.addResourceNode(10, 10);

    if(resourceNodeList.size() != 1) return false;
    cv.addResourceNode(10, 10);
    if(resourceNodeList.size() != 1) return false;

    cv.addResourceNode(10, 20);
    if(resourceNodeList.size() != 2) return false;

    return true;
  }

  /*
   * Case25: 最初のモードがちゃんと決められるか
   */
  bool testCase25(){
    cv.stageInitialize();

    cv.addResourceNode(10, 10);

    Unit *unit = cv.createDummyUnit(100, 10, 10, 2000, WORKER);

    if(unit->resourceY != 10 || unit->resourceX != 10) return false;
    if(unit->mode != PICKING) return false;

    gameStage.field[10][10].myUnitCount[WORKER] = 6;
    unit = cv.createDummyUnit(101, 10, 10, 2000, WORKER);
    if(unit->mode == PICKING) return false;

    return true;
  }

  /*
   * Case26: 行動の優先順位が設定されているかどうか
   */
  bool testCase26(){
    cv.stageInitialize(); 

    Unit *worker = cv.createDummyUnit(100, 10, 10, 2000, WORKER);
    int workerMovePirority = cv.directUnitMovePriority(worker);

    Unit *village = cv.createDummyUnit(101, 11, 11, 2000, VILLAGE);
    int villageMovePriority = cv.directUnitMovePriority(village);

    Unit *castel = cv.createDummyUnit(102, 20, 20, 50000, CASTEL);
    int castelMovePriority = cv.directUnitMovePriority(castel);

    Unit *worker2 = cv.createDummyUnit(103, 50, 50, 2000, WORKER);
    int workerMovePirority2 = cv.directUnitMovePriority(worker2);

    Unit *assasin = cv.createDummyUnit(104, 40, 40, 5000, ASSASIN);
    assasin->role = LEADER;
    int leaderMovePriority = cv.directUnitMovePriority(assasin);

    Unit *assasin2 = cv.createDummyUnit(105, 40, 40, 5000, ASSASIN);
    assasin2->role = COMBATANT;
    int combatMovePriority = cv.directUnitMovePriority(assasin2);

    Unit *colliery = cv.createDummyUnit(106, 30, 30, 20000, VILLAGE);
    colliery->role = COLLIERY;
    int collieryMovePriority = cv.directUnitMovePriority(colliery);

    if(workerMovePirority > villageMovePriority) return false;
    if(villageMovePriority < castelMovePriority) return false;
    if(workerMovePirority >= workerMovePirority2) return false;
    if(leaderMovePriority < villageMovePriority) return false;
    if(leaderMovePriority < combatMovePriority) return false;
    if(villageMovePriority < collieryMovePriority) return false;

    return true;
  }

  /*
   * Case27: 敵ユニットの追加ができているかどうか
   */
  bool testCase27(){
    cv.stageInitialize();

    cv.addEnemyUnit(100, 10, 10, 2000, WORKER);
    if(enemyActiveUnitList.size() != 1) return false;

    cv.addEnemyUnit(101, 80, 80, 50000, CASTEL);
    if(enemyActiveUnitList.size() != 2) return false;
    if(enemyCastelCoordY != 80 && enemyCastelCoordX != 80) return false;

    return true;
  }

  /*
   * Case28: 試合状況が確認できているかどうか
   */
  bool testCase28(){
    cv.stageInitialize();

    cv.updateGameSituation();
    if(gameStage.gameSituation != OPENING) return false;

    cv.addEnemyUnit(100, 30, 30, 2000, WORKER);
    cv.updateGameSituation();
    if(gameStage.gameSituation != WARNING) return false;

    cv.createDummyEnemyUnit(0, 0, 0, 50000, CASTEL);
    cv.updateGameSituation();
    if(gameStage.gameSituation != DANGER) return false;

    return true;
  }

  /*
   * Case29: ユニットの役割がちゃんと割り振れているかどうか
   */
  bool testCase29(){
    cv.stageInitialize(); 

    Unit *worker = cv.createDummyUnit(100, 10, 10, 2000, WORKER);

    if(worker->role != WORKER) return false;

    gameStage.field[10][10].myUnitCount[ASSASIN] = 1;
    Unit *assasin = cv.createDummyUnit(101, 10, 10, 5000, ASSASIN);
    if(assasin->role != LEADER) return false;

    gameStage.field[10][10].myUnitCount[ASSASIN] = 2;
    Unit *assasin2 = cv.createDummyUnit(102, 10, 10, 5000, ASSASIN);
    if(assasin2->role != COMBATANT) return false;
    if(assasin2->leaderId != 101) return false;

    return true;
  }

  /*
   * Case30: 目的地のリセットが出来ているかどうか
   */
  bool testCase30(){
    cv.stageInitialize();

    Unit *unit = cv.createDummyUnit(100, 10, 10, 5000, WORKER);
    cv.resetDestPoint(unit);

    if(unit->destY != UNDEFINED || unit->destX != UNDEFINED) return false;

    return true;
  }

  /*
   * Case31: 破壊モードに移行出来ているかどうか
   */
  bool testCase31(){
    cv.stageInitialize();
    gameStage.field[10][10].myUnitCount[ASSASIN] = 1;

    Unit *assasin = cv.createDummyUnit(100, 10, 10, 5000, ASSASIN);
    if(assasin->role != LEADER) return false;

    assasin->troopsCount = assasin->troopsLimit;
    assasin->mode = cv.directUnitMode(assasin);

    if(assasin->mode != DESTROY) return false;

    return true;
  }

  /*
   * Case32: 自軍のユニットが生きているかの確認
   */
  bool testCase32(){
    cv.stageInitialize();

    myActiveUnitList.insert(100);

    if(!cv.isAlive(100)) return false;
    if(cv.isAlive(101)) return false;

    return true;
  }

  /*
   * Case33: 行動の評価値がちゃんとしている
   */
  bool testCase33(){
    cv.stageInitialize();

    Unit *assasin = cv.createDummyUnit(0, 10, 10, 5000, ASSASIN);
    if(assasin->role != LEADER) return false;

    Unit *assasin2 = cv.createDummyUnit(1, 10, 10, 5000, ASSASIN);
    if(assasin2->role != COMBATANT) return false;

    return true;
  }

  /*
   * Case34: ダミーユニットの作成が出来ているかどうか
   */
  bool testCase34(){
    cv.stageInitialize();

    Unit *assasin = cv.createDummyUnit(0, 10, 10, 5000, ASSASIN);
    if(gameStage.field[10][10].myUnitCount[ASSASIN] != 1) return false;

    Unit *assasin2 = cv.createDummyUnit(1, 10, 10, 5000, ASSASIN);
    if(gameStage.field[10][10].myUnitCount[ASSASIN] != 2) return false;
    Unit *assasin3 = cv.createDummyUnit(2, 11, 10, 5000, ASSASIN);
    Unit *assasin4 = cv.createDummyUnit(3, 10, 11, 5000, ASSASIN);

    if(assasin->mode != STAY) return false;
    if(assasin2->mode != STAY) return false;
    if(cv.calcManhattanDist(assasin2->y, assasin2->x, assasin->y, assasin->x) != 0) return false;

    assasin->mode = STAY;
    assasin2->mode = STAY;
    assasin3->mode = STAY;
    assasin4->mode = STAY;

    assasin3->role = COMBATANT;
    assasin3->leaderId = 0;

    assasin4->role = COMBATANT;
    assasin4->leaderId = 0;

    if(assasin->role != LEADER) return false;
    if(assasin2->role != COMBATANT) return false;
    if(assasin3->role != COMBATANT) return false;
    if(assasin4->role != COMBATANT) return false;

    if(cv.calcManhattanDist(assasin2->y, assasin2->x, assasin->y, assasin->x) != 0) return false;
    if(cv.calcManhattanDist(assasin3->y, assasin3->x, assasin->y, assasin->x) != 1) return false;

    assasin->troopsCount = 11;
    gameStage.field[10][10].myUnitCount[ASSASIN] = 11;

    gameStage.field[11][10].myUnitCount[ASSASIN] = 5;

    return true;
  }

  /*
   * リーダがDESTROYになったときに戦闘員もDESTROYになる
   */
  bool testCase35(){
    cv.stageInitialize();

    Unit *leader = cv.createDummyUnit(0, 10, 10, 5000, ASSASIN);
    Unit *combat = cv.createDummyUnit(1, 10, 10, 5000, ASSASIN);

    if(leader->role != LEADER) return false;
    if(combat->role != COMBATANT) return false;

    leader->mode = DESTROY;
    combat->mode = cv.directUnitMode(combat);

    if(combat->mode != DESTROY) return false;

    return true;
  }

  /*
   * 複数リーダが居るときには一番近いリーダに
   */
  bool testCase36(){
    cv.stageInitialize();

    cv.createDummyUnit(0, 10, 10, 5000, ASSASIN);
    cv.createDummyUnit(1, 80, 80, 5000, ASSASIN);

    Unit *combat = cv.createDummyUnit(2, 80, 80, 5000, ASSASIN);

    if(combat->leaderId != 1) return false;

    return true;
  }

  /*
   * Case37: 城からの攻撃を受けたかどうかの判定
   */
  bool testCase37(){
    cv.stageInitialize();

    Unit *assasin = cv.createDummyUnit(0, 10, 10, 5000, ASSASIN);
    if(cv.isCastelDamage(assasin)) return false;

    assasin->hp = 4900;
    cv.addEnemyUnit(1, 11, 11, 5000, WORKER);

    if(cv.isCastelDamage(assasin)) return false;

    assasin->hp = 4950;
    cv.createDummyUnit(2, 10, 10, 5000, ASSASIN);

    if(cv.isCastelDamage(assasin)) return false;

    assasin->hp = 4990;
    for(int i = 3; i < 11; i++){
      cv.createDummyUnit(i, 10, 10, 5000, ASSASIN);
    }
    if(cv.isCastelDamage(assasin)) return false;

    cv.createDummyUnit(11, 10, 10, 5000, ASSASIN);
    if(cv.isCastelDamage(assasin)) return false;

    assasin->y = 85;
    assasin->x = 85;

    cv.addEnemyUnit(12, 90, 90, 50000, CASTEL);
    if(!cv.isCastelDamage(assasin)) return false;

    // reset
    cv.stageInitialize();
    Unit *worker = cv.createDummyUnit(0, 63, 90, 1900, WORKER);
    cv.addEnemyUnit(1, 67, 96, 50000, CASTEL);
    if(!cv.isCastelDamage(worker)) return false;

    // reset
    cv.stageInitialize();
    Unit *worker1 = cv.createDummyUnit(0, 94, 17, 1550, WORKER);
    worker1->beforeHp = 1566;

    Unit *village = cv.createDummyUnit(1, 94, 17, 19650, VILLAGE);
    village->beforeHp = 19666;

    Unit *worker2 = cv.createDummyUnit(2, 94, 17, 1700, WORKER);
    worker2->beforeHp = 1716;

    Unit *worker3 = cv.createDummyUnit(3, 94, 17, 1766, WORKER);
    worker3->beforeHp = 1782;

    Unit *worker4 = cv.createDummyUnit(4, 94, 17, 1816, WORKER);
    worker4->beforeHp = 1832;

    Unit *worker5 = cv.createDummyUnit(5, 94, 17, 1856, WORKER);
    worker5->beforeHp = 1872;

    cv.addEnemyUnit(6, 94, 17, 20000, VILLAGE);
    if(enemyActiveUnitList.size() != 1) return false;
    if(cv.isCastelDamage(worker1)) return false;

    return true;
  }

  /*
   * Case38: 自軍の数を数えることができる
   */
  bool testCase38(){
    cv.stageInitialize();

    cv.createDummyUnit(0, 10, 10, 5000, ASSASIN);
    cv.createDummyUnit(1, 10, 10, 5000, ASSASIN);

    int myUnitCount = cv.countMyUnit(10, 10, 2);

    if(myUnitCount != 2) return false;

    for(int i = 2; i < 11; i++){
      cv.createDummyUnit(i, 10, 10, 5000, ASSASIN);
    }

    myUnitCount = cv.countMyUnit(10, 10, 2);
    if(myUnitCount != 10) return false;

    cv.createDummyUnit(20, 63, 90, 2000, WORKER);
    myUnitCount = cv.countMyUnit(67, 96, 10);
    if(myUnitCount != 1) return false;

    return true;
  }

  /*
   * Case39: 敵の城の候補地を出す
   */
  bool testCase39(){
    cv.stageInitialize();

    cv.setCastelPointList(20, 20);

    if(gameStage.enemyCastelPointList.size() != 0) return false;

    cv.setCastelPointList(92, 58);

    return true;
  }

  /*
   * Case40: 近くの壁との距離が測れる
   */
  bool testCase40(){
    cv.stageInitialize();

    if(cv.calcNearWallDistance(0,0) != 0) return false;
    if(cv.calcNearWallDistance(3,2) != 2) return false;
    if(cv.calcNearWallDistance(10,20) != 10) return false;
    if(cv.calcNearWallDistance(99,98) != 0) return false;

    return true;
  }

  /*
   * Case41: 敵の生存確認ができる
   */
  bool testCase41(){
    cv.stageInitialize();
    cv.addEnemyUnit(0, 10, 10, 2000, WORKER);
    Unit *enemy = &unitList[0];
    Node *node = &gameStage.field[10][10];

    enemy->timestamp = -1;
    node->timestamp = turn;
    if(enemyActiveUnitList.size() != 1) return false;

    cv.enemyUnitSurvivalCheck();

    if(enemyActiveUnitList.size() != 0) return false;

    return true;
  }

  /*
   * Case42: 収入のカウントが出来ている
   */
  bool testCase42(){
    cv.stageInitialize();

    cv.addResourceNode(10, 10);
    cv.updateIncomeResource();
    if(gameStage.incomeResource != 10) return 0;

    gameStage.field[10][10].myUnitCount[WORKER] = 1;
    cv.updateIncomeResource();
    if(gameStage.incomeResource != 11) return 0;

    gameStage.field[10][10].myUnitCount[WORKER] = 6;
    cv.updateIncomeResource();
    if(gameStage.incomeResource != 15) return 0;

    return true;
  }

  /*
   * Case43: 敵の城である可能性があるかどうかを確かめる
   */
  bool testCase43(){
    cv.stageInitialize();

    cv.addEnemyUnit(0, 90, 72, 50000, CASTEL);
    gameStage.field[95][67].searched = true;

    if(!cv.isCastelPoint(90, 72)) return false;
    if(!cv.isCastelPoint(99, 61)) return false;

    return true;
  }

  /*
   * Case44: 敵の攻撃を受ける範囲をマークつけることが出来る
   */
  bool testCase44(){
    cv.stageInitialize();

    Unit *enemy = cv.createDummyEnemyUnit(0, 50, 50, 2000, WORKER);
    cv.checkEnemyMark(enemy);

    Node *node = cv.getNode(50, 50);

    if(node->enemyAttackCount[WORKER] != 1) return false;

    node = cv.getNode(50, 53);
    if(node->enemyAttackCount[WORKER] != 0) return false;

    node = cv.getNode(50, 54);
    if(node->enemyAttackCount[WORKER] != 0) return false;

    return true;
  }

  /*
   * Case45: 敵の攻撃のダメージの計算を行う
   */
  bool testCase45(){
    cv.stageInitialize();

    Unit *enemy   = cv.createDummyEnemyUnit(0, 50, 50, 5000, KNIGHT);
    Unit *worker  = cv.createDummyUnit(1, 50, 49, 2000, WORKER);
    Unit *knight  = cv.createDummyUnit(2, 50, 48, 5000, KNIGHT);
    Unit *fighter = cv.createDummyUnit(3, 50, 47, 5000, FIGHTER);
    Unit *assasin = cv.createDummyUnit(4, 50, 46, 5000, ASSASIN);

    cv.checkEnemyMark(enemy);

    if(cv.calcReceivedCombatDamage(worker) != 100) return false;
    if(cv.calcReceivedCombatDamage(knight) != 500) return false;
    if(cv.calcReceivedCombatDamage(fighter) != 0) return false;
    if(cv.calcReceivedCombatDamage(assasin) != 0) return false;

    return true;
  }

  /*
   * Case46: 資源マスが占領されているかどうかを調べる
   */
  bool testCase46(){
    cv.stageInitialize();

    cv.addResourceNode(50, 50);
    if(cv.isOccupied(50, 50)) return false;

    for(int i = 0; i < 5; i++){
      cv.addEnemyUnit(i, 50, 50, 2000, WORKER);
    }
    if(!cv.isOccupied(50, 50)) return false;

    cv.addResourceNode(10, 10);
    cv.createDummyEnemyUnit(5, 10, 10, 20000, VILLAGE);

    if(!cv.isOccupied(10,10)) return false;

    cv.addResourceNode(20,20);
    cv.createDummyEnemyUnit(6, 20, 20, 20000, BASE);

    //if(!cv.isOccupied(20,20)) return false;

    return true;
  }
  
  /*
   * 役割が変わっているかどうか
   */
  bool testCase47(){
    cv.stageInitialize();

    Unit *village = cv.createDummyUnit(0, 10, 10, 20000, VILLAGE);
    Node *node = cv.getNode(10, 10);

    if(village->role != VILLAGE) return false;

    node->myUnitCount[WORKER] = 5;
    cv.updateUnitRole();

    if(village->role != COLLIERY) return false;

    return true;
  }

  /*
   * 敵を倒せる判定が出来ているかどうか
   */
  bool testCase48(){
    cv.stageInitialize();

    Unit *enemy = cv.createDummyEnemyUnit(0, 10, 10, 2000, WORKER);
    cv.createDummyUnit(1, 10, 12, 5000, ASSASIN);

    if(cv.isKilled(enemy)) return false;

    cv.createDummyUnit(2, 10, 12, 5000, ASSASIN);
    if(!cv.isKilled(enemy)) return false;

    Unit *enemy2 = cv.createDummyEnemyUnit(3, 10, 10, 2000, WORKER);
    if(cv.isKilled(enemy)) return false;

    return true;
  }

  /*
   * 倒せる敵の数を数えられているかどうか
   */
  bool testCase49(){
    cv.stageInitialize();

    cv.createDummyEnemyUnit(0, 10, 10, 2000, WORKER);

    cv.createDummyUnit(1, 10, 10, 5000, ASSASIN);
    
    if(cv.canKillEnemyCount(10, 10, 2) != 0) return false;

    cv.createDummyUnit(2, 10, 11, 5000, ASSASIN);

    if(cv.canKillEnemyCount(10, 10, 2) != 1) return false;

    return true;
  }

  /*
   * 座標系が逆になっているかどうか
   */
  bool testCase50(){
    cv.stageInitialize();

    firstPlayer = false;
    Coord coord1 = cv.reverseCoord(0,0);
    Coord coord2 = cv.reverseCoord(10,10);
    Coord coord3 = cv.reverseCoord(99,99);
    Coord coord4 = cv.reverseCoord(50,50);

    if(coord1.y != 99 || coord1.x != 99) return false;
    if(coord2.y != 89 || coord2.x != 89) return false;
    if(coord3.y != 0 || coord3.y != 0) return false;
    if(coord4.y != 49 || coord4.y != 49) return false;

    return true;
  }

  /*
   *  敵のkの値が更新できているかどうか
   */
  bool testCase51(){
    cv.stageInitialize();

    cv.createDummyEnemyUnit(0, 10, 10, 2000, WORKER);
    cv.createDummyEnemyUnit(1, 10, 11, 5000, KNIGHT);
    cv.createDummyEnemyUnit(2, 11, 10, 5000, FIGHTER);
    cv.createDummyEnemyUnit(3, 11, 11, 5000, ASSASIN);

    cv.createDummyUnit(4, 9, 8, 2000, WORKER);

    Node *node = cv.getNode(10, 10);
    Node *node1 = cv.getNode(9, 10);

    cv.updateBattleData();

    if(node->enemyK != 4) return false;
    if(node1->enemyK != 3) return false;
    if(node->myK != 0) return false;
    if(node1->myK != 1) return false;

    return true;
  }

  /*
   * 周囲(攻撃範囲)の敵の種類を把握
   */
  bool testCase52(){
    cv.stageInitialize();

    cv.createDummyEnemyUnit(0, 10, 10, 2000, WORKER);
    cv.createDummyEnemyUnit(1, 10, 11, 5000, KNIGHT);
    cv.createDummyEnemyUnit(2, 11, 10, 5000, FIGHTER);
    cv.createDummyEnemyUnit(3, 11, 11, 5000, ASSASIN);
    cv.createDummyEnemyUnit(4, 10, 9, 5000, KNIGHT);

    Node *node1 = cv.getNode(10, 10);
    Node *node2 = cv.getNode(10, 9);

    cv.updateBattleData();

    if(node1->enemyCountWithinAttackRange[WORKER] != 1) return false;
    if(node1->enemyCountWithinAttackRange[KNIGHT] != 2) return false;
    if(node2->enemyCountWithinAttackRange[KNIGHT] != 2) return false;
    if(node2->enemyCountWithinAttackRange[ASSASIN] != 0) return false;

    return true;
  }

  /*
   * 与えるダメージの計算が出来ている
   */
  bool testCase53(){
    cv.stageInitialize();

    cv.createDummyEnemyUnit(0, 10, 10, 2000, WORKER);
    cv.createDummyEnemyUnit(1, 10, 11, 5000, KNIGHT);
    cv.createDummyEnemyUnit(2, 11, 10, 5000, FIGHTER);
    cv.createDummyEnemyUnit(3, 11, 11, 5000, ASSASIN);
    cv.createDummyEnemyUnit(4, 10, 9, 5000, KNIGHT);

    cv.createDummyUnit(5, 10, 8, 5000, ASSASIN);

    Node *node1 = cv.getNode(10, 10);

    cv.updateBattleData();

    if(node1->attackDamage[ASSASIN] != 700) return false;

    return true;
  }

  /*
   * 受けるダメージの計算が出来ている
   */
  bool testCase54(){
    cv.stageInitialize();

    cv.createDummyEnemyUnit(0, 10, 10, 2000, WORKER);
    cv.createDummyEnemyUnit(1, 10, 11, 5000, KNIGHT);
    cv.createDummyEnemyUnit(2, 11, 10, 5000, FIGHTER);
    cv.createDummyEnemyUnit(3, 11, 11, 5000, ASSASIN);
    cv.createDummyUnit(4, 10, 10, 5000, ASSASIN);
    cv.createDummyUnit(5, 8, 8, 5000, ASSASIN);

    Node *node1 = cv.getNode(10, 10);
    Node *node2 = cv.getNode(9, 9);

    cv.updateBattleData();

    if(node1->receiveDamage[ASSASIN] != 1000) return false;
    if(node2->receiveDamage[ASSASIN] != 50) return false;

    return true;
  }

  /*
   * 敵が占領している資源マスのリストが手に入る
   */
  bool testCase55(){
    cv.stageInitialize();

    cv.addResourceNode(10, 10);

    for(int i = 0; i < 5; i++){
      cv.createDummyEnemyUnit(i, 10, 10, 2000, WORKER);
    }

    cv.updateResourceNodeData();

    if(enemyResourceNodeList.size() != 1) return false;

    return true;
  }

  /*
   * 周りの敵の数を数えることが出来る。
   */
  bool testCase56(){
    cv.stageInitialize();

    cv.createDummyEnemyUnit(0, 10, 10, 2000, ASSASIN);

    if(cv.aroundEnemyUnitCount(10, 10, 1).totalCount != 1) return false;

    cv.createDummyEnemyUnit(1, 11, 11, 2000, ASSASIN);
    if(cv.aroundEnemyUnitCount(10, 10, 2).totalCount != 2) return false;
    if(cv.aroundEnemyUnitCount(10, 10, 1).totalCount != 1) return false;

    return true;
  }

  /*
   * 周りの自軍の数を数えることが出来る。
   */
  bool testCase57(){
    cv.stageInitialize();

    cv.createDummyUnit(0, 10, 10, 2000, WORKER);

    if(cv.aroundMyUnitCount(10, 10, 1).totalCount != 1) return false;

    cv.createDummyUnit(1, 11, 11, 2000, WORKER);
    if(cv.aroundMyUnitCount(10, 10, 2).totalCount != 2) return false;
    if(cv.aroundMyUnitCount(10, 10, 1).totalCount != 1) return false;

    return true;
  }

  /*
   * 次のリーダが探索できているかどうか
   */
  bool testCase58(){
    cv.stageInitialize();

    Unit *assasin1 = cv.createDummyUnit(0, 10, 10, 5000, ASSASIN);
    cv.createDummyUnit(1, 20, 10, 5000, ASSASIN);
    cv.createDummyUnit(2, 80, 80, 5000, ASSASIN);
    cv.createDummyUnit(3, 11, 11, 20000, BASE);

    int newLeaderId = cv.searchNextLeader(assasin1);

    if(newLeaderId != 1) return false;

    return true;
  }

  /*
   * 1P側か2P側かを判断できる
   */
  bool testCase59(){
    cv.stageInitialize();

    cv.createDummyUnit(0, 10, 10, 50000, CASTEL); 
    if(!cv.isFirstPlayer()) return false;

    cv.createDummyUnit(0, 90, 90, 50000, CASTEL); 
    if(cv.isFirstPlayer()) return false;

    return true;
  }
};

int main(){
  Codevs cv;
  CodevsTest cvt;

  cv.run();
  cvt.runTest();

  return 0;
}
