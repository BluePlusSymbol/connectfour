// gametheory.cpp
// ゲーム理論のアルゴリズムを用いてConnect Four (重力付き四目並べ) を部分的に解く
// main関数でPlayerクラスのインスタンスとしてプレイヤーp, q (それぞれ先手、後手) を作成し、ゲームを行う

#include <bits/stdc++.h>
#define DEBUG 1
#define ERROR {if (EXIT_IF_ERROR == 1) exit(1);}
using namespace std;

// 定数
const int EXIT_IF_ERROR = 1;
const int PRINT_BOARD = 1;
const int SHOW_EVAL = 0;

const int INF = 100;

const int ROW = 6;
const int COL = 7;
const int SIZE = ROW*COL;

// 右、左下、下、右下
const int DIR = 4;
const int DR[] = {0, 1, 1, 1};
const int DC[] = {1, -1, 0, 1};

const int PLAYER_O = 0;
const int PLAYER_X = 1;
const int NO_ONE = 2;
const int DRAW = 3;

const char EMPTY = '.';
const char SYMBOL[] = {'o', 'x'};
const map<char, int> PLAYER = {{'o', 0}, {'x', 1}};

// 中央の列から探索
const int ORDER[] = {3, 2, 4, 1, 5, 0, 6};

random_device rd{};
mt19937 mt(rd());

// ボード
class Board {
    private:
    vector<vector<char>> board;
    vector<int> height;

    public:
    Board() {
        board.resize(ROW, vector<char>(COL, '.'));
        height.assign(COL, 0);
    }

    // マス(r, c)を返す
    char getSquare(int r, int c) const {
        if (outOfBounds(r, c)) {
            cerr << "getSquare: Invalid square (" << r << ", " << c << ")" << endl;
            ERROR;
            return '?';
        }
        return board[r][c];
    }

    // 列cの高さを返す
    int getHeight(int c) const {
        if (c < 0 || COL <= c) {
            cerr << "getHeight: Invalid column " << c << endl;
            ERROR;
            return ROW;
        }
        return height[c];
    }

    // 盤面を返す
    vector<vector<char>> getBoard() const {return board;}

    // ボードの出力
    void printBoard() const {
        for (int i = ROW-1; i >= 0; i--) {
            for (int j = 0; j < COL; j++) {
                cout << board[i][j];
            }
            cout << endl;
        }
        cout << endl;
    }

    // 列colにplayerがコマを落とす
    void drop(int col, int player) {
        if (col < 0 || COL <= col) {
            cerr << "drop: Invalid column " << col << endl;
            ERROR;
            return;
        }

        if (height[col] >= ROW) {
            std::cerr << "drop: Column " << col << " is full" << endl;
            ERROR;
            return;
        }

        board[height[col]][col] = SYMBOL[player];
        height[col]++;
    }

    // 列colから1個コマを取り除く
    // 先読みするときに使用
    void remove(int col) {
        if (col < 0 || COL <= col) {
            cerr << "remove: Invalid column " << col << endl;
            ERROR;
            return;
        }

        if (height[col] <= 0) {
            std::cerr << "remove: Column " << col << " is empty" << endl;
            ERROR;
            return;
        }

        board[height[col]-1][col] = EMPTY;
        height[col]--;
    }

    // 盤面を反転
    static void invert(vector<vector<char>>& state) {
        for (int i = 0; i < ROW; i++) {
            for (int j = 0; j < COL; j++) {
                if (state[i][j] == SYMBOL[PLAYER_O]) state[i][j] = SYMBOL[PLAYER_X];
                else if (state[i][j] == SYMBOL[PLAYER_X]) state[i][j] = SYMBOL[PLAYER_O];
            }
        }
    }

    // 盤面を反転
    void invertBoard() {invert(board);}

    // マス(r, c)が範囲外か判定
    static int outOfBounds(int r, int c) {
        if (r < 0 || ROW <= r) return true;
        if (c < 0 || COL <= c) return true;
        return false;
    }

    // ゲームが終了しているか判定
    int checkWinner() const {
        int i, j, dir, k;
        bool b;

        for (i = 0; i < ROW; i++) {
            for (j = 0; j < COL; j++) {
                if (board[i][j] == EMPTY) continue;

                for (dir = 0; dir < DIR; dir++) {
                    if (outOfBounds(i+3*DR[dir], j+3*DC[dir])) continue;

                    b = true;

                    for (k = 0; k < 3; k++) {
                        int r = i + k*DR[dir];
                        int c = j + k*DC[dir];

                        if (board[r][c] == EMPTY) {b = false; break;}
                        if (board[r][c] != board[r + DR[dir]][c + DC[dir]]) {b = false; break;}
                    }

                    if (b) {
                        auto it = PLAYER.find(board[i][j]);
                        return it->second;
                    }
                }
            }
        }

        for (i = 0; i < ROW; i++) {
            for (j = 0; j < COL; j++) {
                if (board[i][j] == EMPTY) return NO_ONE;
            }
        }

        return DRAW;
    }
};

// プレイヤ
class Player{
    public:
    virtual int move(Board& board, int turn) {return 0;};
};

// 人間プレイヤ
// 標準入力で操作
class Manual: public Player {
    public:
    int move(Board& board, int turn) override {
        int c;
        while (1) {
            cin >> c;
            if (board.getHeight(c) < ROW) return c;
            else cout << "Column " << c << " is full or invalid" << endl;
        }
    }
};

// ランダムプレイヤ
// 合法手を等確率でランダムに選択
class Random: public Player {
    public:
    int move(Board& board, int turn) override {
        vector<int> moves;
        for (int j = 0; j < COL; j++) {
            if (board.getHeight(j) < ROW) moves.push_back(j);
        }
        int next = moves[mt() % moves.size()];

        return next;
    }
};

// 一手先読み
// コマを落として勝てる列があるならば、そこに落として勝つ
// そのような列がなく、放置すると次のターンで敗北する列があるならば、そこに落とす
// そのような列もないならば、このターンで相手に足場を作り次のターンで敗北 (自滅) しないように落とす
// 自滅せざるを得ない場合は諦める
class OneMove: public Player {
    public:
    int move(Board& board, int turn) override {
        bool b;

        // コマを落として勝てる列があるならば、そこに落として勝つ
        for (int j = 0; j < COL; j++) {
            if (board.getHeight(j) < ROW) {
                b = false;

                board.drop(j, turn);
                if (board.checkWinner() == turn) b = true;
                board.remove(j);

                if (b) return j;
            }
        }

        // そのような列がなく、放置すると次のターンで敗北する列があるならば、そこに落とす
        for (int j = 0; j < COL; j++) {
            if (board.getHeight(j) < ROW) {
                b = false;

                board.drop(j, turn^1);
                if (board.checkWinner() == (turn^1)) b = true;
                board.remove(j);

                if (b) return j;
            }
        }

        // そのような列もないならば、このターンで相手に足場を作り次のターンで敗北 (自滅) しないように落とす
        vector<int> moves;
        for (int j = 0; j < COL; j++) {
            if (board.getHeight(j) <= ROW-2) {
                b = true;

                board.drop(j, turn);
                board.drop(j, turn^1);
                if (board.checkWinner() == (turn^1)) b = false;
                board.remove(j);
                board.remove(j);

                if (b) moves.push_back(j);
            }
            else if (board.getHeight(j) == ROW-1) {
                b = false;

                board.drop(j, turn);
                if (board.checkWinner() == NO_ONE) b = true;
                board.remove(j);

                if (b) moves.push_back(j);
            }
        }

        if (moves.size() != 0) {
            int next = moves[mt() % moves.size()];
            return next;
        }

        // 自滅せざるを得ない場合は諦める
        moves.clear();
        for (int j = 0; j < COL; j++) {
            if (board.getHeight(j) < ROW) moves.push_back(j);
        }
        
        int next = moves[mt() % moves.size()];
        return next;
    }
};

// negamax法を用いるプレイヤ
class Negamax: public Player {
    private:
    long long nodes;
    long long depth;

    public:
    Negamax(int d) {
        nodes = 0;
        depth = d;
    }

    long long getNodes() {return nodes;}

    int move(Board& board, int turn) override {
        if (turn == PLAYER_X) board.invertBoard();
        int next = negamax(board, depth, true);
        if (turn == PLAYER_X) board.invertBoard();

        return next;
    }

    // 盤面boardから深さdepthのnegamax
    // moveから呼び出すときはdepth > 0でなければならない
    // parent == falseのとき、評価値を返し、
    // parent == trueのとき、コマを落とすべき列を返す
    int negamax(Board& board, int depth, bool parent) {
        int maxscore, v, next;
        vector<int> score(COL, -3*INF);

        maxscore = -2*INF;
        for (int j = 0; j < COL; j++) {
            if (board.getHeight(j) >= ROW) continue;

            board.drop(j, PLAYER_O);

            if (depth > 1) {
                v = eval(board.getBoard());
                board.invertBoard();
                if (-INF < v && v < INF) v = -negamax(board, depth-1, false);
                board.invertBoard();
            }
            else v = eval(board.getBoard());

            board.remove(j);

            score[j] = v;
            if (v > maxscore) maxscore = v;
        }
        
        if (parent) {
            if (SHOW_EVAL) {
                for (int j = 0; j < COL; j++) cout << score[j] << " ";
                cout << endl;
            }
            
            vector<int> moves;
            for (int j = 0; j < COL; j++) {
                if (score[j] == maxscore) moves.push_back(j);
            }

            next = moves[mt() % moves.size()];
            return next;
        }

        return maxscore;
    }

    // 評価関数
    // 勝利ならば 100 + (空きマスの数)
    // 敗北ならば 100 + (空きマスの数)
    // そうでないならば (先手が作る可能性がある4連の場所の数) - (後手のそれ)
    int eval(const vector<vector<char>>& state) {
        int score, empty, i, j, dir, k;
        int a, b;

        empty = 0;
        for (i = 0; i < ROW; i++) {
            for (j = 0; j < COL; j++) {
                if (state[i][j] == EMPTY) empty++;
            }
        }

        score = 0;
        for (i = 0; i < ROW; i++) {
            for (j = 0; j < COL; j++) {
                for (dir = 0; dir < DIR; dir++) {
                    if (Board::outOfBounds(i+3*DR[dir], j+3*DC[dir])) continue;

                    a = 0;
                    b = 0;

                    for (k = 0; k < 4; k++) {
                        int r = i + k*DR[dir];
                        int c = j + k*DC[dir];

                        if (state[r][c] == SYMBOL[PLAYER_O]) a++;
                        if (state[r][c] == SYMBOL[PLAYER_X]) b++;
                    }

                    if (a == 4) {return INF+empty;}
                    if (b == 4) {return -(INF+empty);}

                    if (b == 0) score++;
                    if (a == 0) score--;
                }
            }
        }

        return score;
    }
};

// alpha-beta pruningなどで高速化
class AlphaBeta: public Player {
    private:
    long long nodes;
    long long depth;

    public:
    AlphaBeta(int d) {
        nodes = 0;
        depth = d;
    }

    long long getNodes() {return nodes;}

    int move(Board& board, int turn) override {
        if (turn == PLAYER_X) board.invertBoard();
        int next = negamax(board, depth, true, -4*INF, 4*INF);
        if (turn == PLAYER_X) board.invertBoard();
        
        return next;
    }

    // 盤面boardから深さdepthのnegamax
    // moveから呼び出すときはdepth > 0でなければならない
    // parent == falseのとき、評価値を返し、
    // parent == trueのとき、コマを落とすべき列を返す
    int negamax(Board& board, int depth, bool parent, int alpha, int beta) {
        int maxscore, v, next;
        vector<int> score(COL, -3*INF);

        maxscore = -2*INF;
        for (int j = 0; j < COL; j++) {
            if (board.getHeight(ORDER[j]) >= ROW) continue;

            board.drop(ORDER[j], PLAYER_O);

            if (depth > 1) {
                v = eval(board.getBoard());
                board.invertBoard();
                if (-INF < v && v < INF) v = -negamax(board, depth-1, false, -beta, -alpha);
                board.invertBoard();
            }
            else v = eval(board.getBoard());

            board.remove(ORDER[j]);

            score[ORDER[j]] = v;
            if (v > maxscore) maxscore = v;

            if (v > beta) {
                break;
                if (parent) return ORDER[j];
                return v;
            }
            if (v > alpha) alpha = v;
        }
        
        if (parent) {
            if (SHOW_EVAL) {
                for (int j = 0; j < COL; j++) cout << score[j] << " ";
                cout << endl;
            }
            
            vector<int> moves;
            for (int j = 0; j < COL; j++) {
                if (score[j] == maxscore) moves.push_back(j);
            }

            next = moves[mt() % moves.size()];
            return next;
        }

        return maxscore;
    }

    // 評価関数
    // 勝利ならば 100 + (空きマスの数)
    // 敗北ならば -(100 + (空きマスの数))
    // そうでないならば (先手が作る可能性がある4連の場所の数) - (後手のそれ)
    int eval(const vector<vector<char>>& state) {
        int score, empty, i, j, dir, k;
        int a, b;

        empty = 0;
        for (i = 0; i < ROW; i++) {
            for (j = 0; j < COL; j++) {
                if (state[i][j] == EMPTY) empty++;
            }
        }

        score = 0;
        for (i = 0; i < ROW; i++) {
            for (j = 0; j < COL; j++) {
                for (dir = 0; dir < DIR; dir++) {
                    if (Board::outOfBounds(i+3*DR[dir], j+3*DC[dir])) continue;

                    a = 0;
                    b = 0;

                    for (k = 0; k < 4; k++) {
                        int r = i + k*DR[dir];
                        int c = j + k*DC[dir];

                        if (state[r][c] == SYMBOL[PLAYER_O]) a++;
                        if (state[r][c] == SYMBOL[PLAYER_X]) b++;
                    }

                    if (a == 4) {return INF+empty;}
                    if (b == 4) {return -(INF+empty);}

                    if (b == 0) score++;
                    if (a == 0) score--;
                }
            }
        }

        return score;
    }
};

void initPlayer(Player*& p) {
    int type, depth;

    cin >> type >> depth;
    
    if (type == 0) {
        p = new Manual();
    }
    else if (type == 1) {
        p = new Random();
    }
    else if (type == 2) {
        p = new OneMove();
    }
    else if (type == 3) {
        p = new Negamax(depth);
    }
    else if (type == 4) {
        p = new AlphaBeta(depth);
    }
    else {
        cerr << "initPlayer: Unable to initialize a player (invalid player type)" << type << endl;
        exit(1);
    }
}

int main(void) {
    chrono::system_clock::time_point start, end;
    start = chrono::system_clock::now();

    vector<Player*> players(2);
    initPlayer(players[0]);
    initPlayer(players[1]);
    
    int N;
    cin >> N;

    int wins = 0;
    int losses = 0;
    int draws = 0;
    int turn, i, j;

    for (i = 0; i < N; i++) {
        Board board = Board();
        turn = 0;
        while (1) {
            int next = players[turn]->move(board, turn);
            
            board.drop(next, turn);

            if (PRINT_BOARD) board.printBoard();

            int winner = board.checkWinner();
            if (winner != NO_ONE) {
                if (winner == PLAYER_O) wins++;
                if (winner == PLAYER_X) losses++;
                if (winner == DRAW) draws++;

                if (PRINT_BOARD) cout << wins << " " << losses << " " << draws << endl;
                break;
            }

            turn ^= 1;
        }
    }

    cout << wins << " " << losses << " " << draws << endl;

    end = chrono::system_clock::now();
    double elapsed = chrono::duration_cast<chrono::milliseconds>(end-start).count();
    cout << elapsed << endl;
}