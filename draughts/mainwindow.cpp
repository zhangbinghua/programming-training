#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "login_dialog.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	rw_socket = nullptr;
	listen_socket = nullptr;

	initWidgets();

	connect(ui->actionStart, SIGNAL(triggered(bool)), this, SLOT(startListening()));
	connect(ui->actionConnect, SIGNAL(triggered(bool)), this, SLOT(connectToServer()));
}

void MainWindow::initWidgets()
{
	window = new QWidget;
	setCentralWidget(window);

	QVBoxLayout *layout = new QVBoxLayout(window);

	chess_board = new ChessBoard;
	layout->addWidget(chess_board);

	info_label = new QLabel;
	layout->addWidget(info_label);

	connect(chess_board, SIGNAL(noAvailChess()), this, SLOT(gameEnd()));
	connect(chess_board, SIGNAL(playerMove(int,int,int,int)),
			this, SLOT(playerMove(int,int,int,int)));
}

void MainWindow::socketError(QTcpSocket::SocketError)
{
}

void MainWindow::connectToServer()
{
	LoginDialog dlg;
	if(dlg.exec() == QDialog::Accepted)
	{
		QHostAddress host_addr;
		if(host_addr.setAddress(dlg.getAddress()))
		{
			rw_socket = new QTcpSocket;
			rw_socket->connectToHost(host_addr, dlg.getPort());
			info_label->setText("Connecting...");
			connect(rw_socket, SIGNAL(connected()), this, SLOT(connectedToHost()));
			connect(rw_socket, SIGNAL(readyRead()), this, SLOT(recvMessage()));
			connect(rw_socket, SIGNAL(error(QAbstractSocket::SocketError)),
					this, SLOT(socketError(QTcpSocket::SocketError)));
		} else {
			QMessageBox::warning(this, "Error", "Error: Invalid address!");
		}
	}
}

void MainWindow::playerMove(int src_x, int src_y, int dest_x, int dest_y)
{
	QString text = QString("%1 %2 %3 %4 %5")
			.arg(OPER_MOVE,
				 QString::number(src_x),
				 QString::number(src_y),
				 QString::number(dest_x),
				 QString::number(dest_y));
	rw_socket->write(text.toUtf8());
//	qDebug() << text;
}

void MainWindow::recvMessage()
{
	QTextStream os(rw_socket);

	QString oper;
	os >> oper;
	if(oper == OPER_MOVE)
	{
		int src_x, src_y, dest_x, dest_y;
		os >> src_x >> src_y >> dest_x >> dest_y;
		chess_board->moveChess(src_x, src_y, dest_x, dest_y);
	} else if(oper == OPER_GIVEUP) {
		gameEnd();
		chess_board->clearMarks();
	}
}

void MainWindow::connectedToHost()
{
	info_label->setText("Connected to " + rw_socket->peerAddress().toString()
						+ ", port " + QString::number(rw_socket->peerPort()));

	chess_board->startGame(DraughtsInfo::white);
}

void MainWindow::acceptConnection()
{
	rw_socket = listen_socket->nextPendingConnection();
	connect(rw_socket, SIGNAL(readyRead()), this, SLOT(recvMessage()));

	info_label->setText("Connected with " + rw_socket->peerAddress().toString()
						+ ", port " + QString::number(rw_socket->peerPort()));

	chess_board->startGame(DraughtsInfo::black);
}

void MainWindow::startListening()
{
	listen_socket = new QTcpServer;
	if(listen_socket->listen())
	{
		connect(listen_socket, SIGNAL(newConnection()), this, SLOT(acceptConnection()));

		info_label->setText("Listening on " + listen_socket->serverAddress().toString()
							+ ", port " + QString::number(listen_socket->serverPort()));
	} else {
		QMessageBox::warning(this, "Error", "Error: " + listen_socket->errorString());
		delete listen_socket;
	}
}

void MainWindow::gameEnd()
{
	if(chess_board->getCurrentPlayer() == chess_board->getPlayer())
	{
		// lost
		QMessageBox::information(this, "Game Over", "You lost!");
	} else {
		// win
		QMessageBox::information(this, "Game Over", "Congratulations! You win!");
	}
}

MainWindow::~MainWindow()
{
	delete ui;
}