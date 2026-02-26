#include "Chess.h"
#include <limits>
#include <cmath>

Chess::Chess()
{
    _grid = new Grid(8, 8);
}

Chess::~Chess()
{
    delete _grid;
}

char Chess::pieceNotation(int x, int y) const
{
    const char *wpieces = { "0PNBRQK" };
    const char *bpieces = { "0pnbrqk" };
    Bit *bit = _grid->getSquare(x, y)->bit();
    char notation = '0';
    if (bit) {
        notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()] : bpieces[bit->gameTag()-128];
    }
    return notation;
}

Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };

    Bit* bit = new Bit();
    // should possibly be cached from player class?
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);

    return bit;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

    startGame();
}

void Chess::FENtoBoard(const std::string& fen) {
    // convert a FEN string to a board
    // FEN is a space delimited string with 6 fields
    // 1: piece placement (from white's perspective)
    // NOT PART OF THIS ASSIGNMENT BUT OTHER THINGS THAT CAN BE IN A FEN STRING
    // ARE BELOW
    // 2: active color (W or B)
    // 3: castling availability (KQkq or -)
    // 4: en passant target square (in algebraic notation, or -)
    // 5: halfmove clock (number of halfmoves since the last capture or pawn advance)

    // Extract piece placement portion (stop at first space for full FEN strings)
    std::string placement = fen.substr(0, fen.find(' '));

    int rankIdx = 0; // 0 = rank 8 (black's back rank), 7 = rank 1 (white's back rank)
    int fileIdx = 0; // 0 = file a, 7 = file h

    for (char c : placement) {
        if (c == '/') {
            rankIdx++;
            fileIdx = 0;
        } else if (c >= '1' && c <= '8') {
            fileIdx += (c - '0'); // skip empty squares
        } else {
            int x = fileIdx;
            int y = 7 - rankIdx; // FEN rank 8 maps to grid y=7, rank 1 to y=0

            int playerNumber;
            ChessPiece piece;

            switch (c) {
                case 'P': playerNumber = 0; piece = Pawn;   break;
                case 'N': playerNumber = 0; piece = Knight; break;
                case 'B': playerNumber = 0; piece = Bishop; break;
                case 'R': playerNumber = 0; piece = Rook;   break;
                case 'Q': playerNumber = 0; piece = Queen;  break;
                case 'K': playerNumber = 0; piece = King;   break;
                case 'p': playerNumber = 1; piece = Pawn;   break;
                case 'n': playerNumber = 1; piece = Knight; break;
                case 'b': playerNumber = 1; piece = Bishop; break;
                case 'r': playerNumber = 1; piece = Rook;   break;
                case 'q': playerNumber = 1; piece = Queen;  break;
                case 'k': playerNumber = 1; piece = King;   break;
                default: fileIdx++; continue;
            }

            Bit* bit = PieceForPlayer(playerNumber, piece);
            bit->setGameTag(playerNumber == 0 ? piece : 128 + piece);
            ChessSquare* square = _grid->getSquare(x, y);
            square->setBit(bit);
            bit->setParent(square);
            bit->moveTo(square->getPosition());
            fileIdx++;
        }
    }
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    // need to implement friendly/unfriendly in bit so for now this hack
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    if (pieceColor == currentPlayer) return true;
    return false;
}

BitboardElement Chess::getPawnMoves(ChessSquare* src, int player)
{
    // Build occupied and enemy bitboards from the current board state
    BitboardElement occupied(0), enemy(0);
    _grid->forEachSquare([&](ChessSquare* sq, int x, int y) {
        if (sq->bit()) {
            int idx = y * 8 + x;
            occupied |= (1ULL << idx);
            if ((sq->bit()->gameTag() & 128) != (player * 128)) {
                enemy |= (1ULL << idx);
            }
        }
    });

    int col = src->getColumn();
    int row = src->getRow();
    int idx = row * 8 + col;

    BitboardElement moves(0);

    if (player == 0) {
        // White pawns move toward increasing row (rank 1 -> rank 8, y=1 -> y=7)
        int pushIdx = idx + 8;
        if (pushIdx < 64 && !(occupied.getData() & (1ULL << pushIdx))) {
            moves |= (1ULL << pushIdx);
            // Double push only from starting row (row 1)
            if (row == 1) {
                int doublePushIdx = idx + 16;
                if (!(occupied.getData() & (1ULL << doublePushIdx))) {
                    moves |= (1ULL << doublePushIdx);
                }
            }
        }
        // Diagonal captures
        if (col > 0 && (enemy.getData() & (1ULL << (idx + 7))))
            moves |= (1ULL << (idx + 7));
        if (col < 7 && (enemy.getData() & (1ULL << (idx + 9))))
            moves |= (1ULL << (idx + 9));
    } else {
        // Black pawns move toward decreasing row (rank 8 -> rank 1, y=6 -> y=0)
        int pushIdx = idx - 8;
        if (pushIdx >= 0 && !(occupied.getData() & (1ULL << pushIdx))) {
            moves |= (1ULL << pushIdx);
            // Double push only from starting row (row 6)
            if (row == 6) {
                int doublePushIdx = idx - 16;
                if (!(occupied.getData() & (1ULL << doublePushIdx))) {
                    moves |= (1ULL << doublePushIdx);
                }
            }
        }
        // Diagonal captures
        if (col > 0 && (enemy.getData() & (1ULL << (idx - 9))))
            moves |= (1ULL << (idx - 9));
        if (col < 7 && (enemy.getData() & (1ULL << (idx - 7))))
            moves |= (1ULL << (idx - 7));
    }

    return moves;
}

BitboardElement Chess::getKnightMoves(ChessSquare* src, int player)
{
    // Build friendly bitboard — knights cannot land on own pieces
    BitboardElement friendly(0);
    _grid->forEachSquare([&](ChessSquare* sq, int x, int y) {
        if (sq->bit() && (sq->bit()->gameTag() & 128) == (player * 128)) {
            friendly |= (1ULL << (y * 8 + x));
        }
    });

    int col = src->getColumn();
    int row = src->getRow();

    BitboardElement moves(0);

    // All 8 L-shaped offsets: (dCol, dRow)
    const int offsets[8][2] = {
        { 1,  2}, { 2,  1}, { 2, -1}, { 1, -2},
        {-1, -2}, {-2, -1}, {-2,  1}, {-1,  2}
    };

    for (auto& off : offsets) {
        int newCol = col + off[0];
        int newRow = row + off[1];
        if (newCol >= 0 && newCol < 8 && newRow >= 0 && newRow < 8) {
            int newIdx = newRow * 8 + newCol;
            if (!(friendly.getData() & (1ULL << newIdx))) {
                moves |= (1ULL << newIdx);
            }
        }
    }

    return moves;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    ChessSquare* srcSquare = static_cast<ChessSquare*>(&src);
    ChessSquare* dstSquare = static_cast<ChessSquare*>(&dst);

    int pieceType = bit.gameTag() & 0x7F;
    int player    = (bit.gameTag() & 128) ? 1 : 0;

    if (pieceType == Pawn) {
        BitboardElement moves = getPawnMoves(srcSquare, player);
        int dstIdx = dstSquare->getSquareIndex();
        return (moves.getData() & (1ULL << dstIdx)) != 0;
    }

    if (pieceType == Knight) {
        BitboardElement moves = getKnightMoves(srcSquare, player);
        int dstIdx = dstSquare->getSquareIndex();
        return (moves.getData() & (1ULL << dstIdx)) != 0;
    }

    return true;
}

void Chess::stopGame()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
}

Player* Chess::ownerAt(int x, int y) const
{
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return nullptr;
    }

    auto square = _grid->getSquare(x, y);
    if (!square || !square->bit()) {
        return nullptr;
    }
    return square->bit()->getOwner();
}

Player* Chess::checkForWinner()
{
    return nullptr;
}

bool Chess::checkForDraw()
{
    return false;
}

std::string Chess::initialStateString()
{
    return stateString();
}

std::string Chess::stateString()
{
    std::string s;
    s.reserve(64);
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
            s += pieceNotation( x, y );
        }
    );
    return s;}

void Chess::setStateString(const std::string &s)
{
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        int index = y * 8 + x;
        char playerNumber = s[index] - '0';
        if (playerNumber) {
            square->setBit(PieceForPlayer(playerNumber - 1, Pawn));
        } else {
            square->setBit(nullptr);
        }
    });
}
