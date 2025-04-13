#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <array>
#include <random>

// Classe TicTacToe
class TicTacToe {
private:
    std::array<std::array<char, 3>, 3> board; // Tabuleiro do jogo
    std::mutex board_mutex; // Mutex para controle de acesso ao tabuleiro
    std::condition_variable turn_cv; // Variável de condição para alternância de turnos
    char current_player; // Jogador atual ('X' ou 'O')
    bool game_over; // Estado do jogo
    char winner; // Vencedor do jogo

public:
    TicTacToe() {
        // Inicializa o tabuleiro e as variáveis do jogo
        for (auto& row : board) { row.fill(' '); }
        this->game_over = false;
        this->winner = ' ';

    }

    void display_board() {
        std::cout << "\n";
        for (int i = 0; i < 3; ++i) {
            std::cout << " ";
            for (int j = 0; j < 3; ++j) {
                std::cout << board[i][j];
                if (j < 2) std::cout << " | ";
            }
            std::cout << "\n";
            if (i < 2) std::cout << "---+---+---\n";
        }
        std::cout << "\n";
    }

    bool make_move(char player, int row, int col) {
        // Implementar a lógica para realizar uma jogada no tabuleiro
        // Utilizar mutex para controle de acesso
        // Utilizar variável de condição para alternância de turnos

         // Bloqueia o mutex
        std::unique_lock<std::mutex> lock(board_mutex);

        // Espera ate que seja a vez deste jogador OU o jogo ja tenha acabado
        turn_cv.wait(lock, [this, player] {
            return current_player == player || game_over;
        });

         // Se o jogo ja terminou enquanto esperava, aborta
            if (this->game_over) return false;

        // Verifica se a casa estah livre
        if (board[row][col] != ' ') return false;

        // Faz a jogada
        board[row][col] = player;

        // Verifica vitoria
        if (check_win(player)) {
            this->game_over = true;
            this->winner = player;
        }
        // Verifica empate
        else if (check_draw()) {
            this->game_over = true;
            this->winner = ' ';  // empate
        }
        // Senao, alterna o jogador atual
        else {
            if (player == 'X') this->current_player = 'O';
            else this->current_player = 'X';
        }

        lock.unlock();
        turn_cv.notify_all();

        return true;
    }

    bool check_win(char player) {
        // Verifica linhas e colunas
        for (int i = 0; i < 3; ++i) {
            // Verifica linhas
            if (board[i][0] == player && board[i][1] == player && board[i][2] == player) return true;
            // Verifica colunas
            if (board[0][i] == player && board[1][i] == player && board[2][i] == player) return true;
        }

        // Verifica diagonal 1
        if (board[0][0] == player && board[1][1] == player && board[2][2] == player) return true;
        // Verifica diagonal 2
        if (board[0][2] == player && board[1][1] == player && board[2][0] == player) return true;
        
        return false;
    }

    bool check_draw() {
        for (const auto& row : board) {
            for (char cell : row) {
                if (cell == ' ')
                    return false;
            }
        }
        return true;
    }

    bool is_game_over() {
        std::lock_guard<std::mutex> lock(board_mutex);
        return this->game_over;
    }

    char get_winner() {
        std::lock_guard<std::mutex> lock(board_mutex);

        // O jogo ainda nao acabou
        if (!this->game_over)return '\0'; 

        // Empate
        if (this->winner == ' ') return 'D';

        return this->winner;
    }
};

// Classe Player
class Player {
private:
    TicTacToe& game; // Referência para o jogo
    char symbol; // Símbolo do jogador ('X' ou 'O')
    std::string strategy; // Estratégia do jogador

public:
    Player(TicTacToe& g, char s, std::string strat) 
        : game(g), symbol(s), strategy(strat) {}

    void play() {
        // Executar jogadas de acordo com a estratégia escolhida

        if (strategy == "sequential") {
            play_sequential();
        } else if (strategy == "random") {
            play_random();
        } else {
            std::cerr << "Estratégia inválida: " << strategy << std::endl;
        }
    }

private:
    void play_sequential() {
        // Enquanto o jogo nao acabar, tenta uma jogada
        while (!game.is_game_over()) {
            bool moved = false;

            // Varre o tabuleiro
            for (int row = 0; row < 3 && !moved; ++row) {
                for (int col = 0; col < 3 && !moved; ++col) {
                    if (game.make_move(symbol, row, col)) {
                        moved = true;  // Jogada bem sucedida. Sai da varredura
                    }
                }
            }
               
            // Se a jogada falhar, cede espaco a outra thread
            std::this_thread::yield();
    }
    }

    void play_random() {
        // Gerador de numeros aleatorios
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, 2);

        // Enquanto o jogo nao acabar, tenta uma jogada
        while (!game.is_game_over()) {
            bool moved = false;

            // Tenta ate conseguir uma jogada válida ou até o jogo acabar
            while (!moved && !game.is_game_over()) {
                int row = dist(gen);
                int col = dist(gen);

                if (game.make_move(symbol, row, col)) {
                    moved = true;  // jogada bem‑sucedida
                } else {
                    // Se a jogada falhar, cede espaco a outra thread
                    std::this_thread::yield();
                }
            }

            // Aposs uma jogada valida, cede o controle 
            std::this_thread::yield();
        }
    }
};

// Função principal
int main() {
    // Inicializar o jogo e os jogadores
    TicTacToe game;

    Player player1(game, 'X', "sequential");
    Player player2(game, 'O', "random");

    // Criar as threads para os jogadores
    std::thread t1(&Player::play, &player1);
    std::thread t2(&Player::play, &player2);

    // Aguardar o término das threads
    t1.join();
    t2.join();

    // Exibir o resultado final do jogo
    std::cout << "\nJogo finalizado!\n";
    game.display_board();

    char winner = game.get_winner();
    if (winner == 'D') {
        std::cout << "O jogo terminou em empate.\n";
    } else {
        std::cout << "O jogador '" << winner << "' venceu!\n";
    }

    return 0;
}
