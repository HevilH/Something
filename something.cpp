#include "something.h"
#include "ui_something.h"
#include <QVBoxLayout>
#include <QMetaType>
#include <QHBoxLayout>
#include <QFileDialog>
#include "BuildIndexThread.h"

Something::Something(QWidget *parent) : QMainWindow(parent), ui(new Ui::Something) {
  ui->setupUi(this);
  this->setWindowTitle("Something");
  this->createUI();
  //this->initEngine();
}

void Something::createUI() {
  setFixedHeight(768);
  setFixedWidth(1024);
  input = new QLineEdit;
  searchBtn = new QPushButton;
  table = new QTableView;
  model = new QStandardItemModel;
  searchBtn->setText("Search");
  QVBoxLayout *vBoxLayout = new QVBoxLayout;
  QHBoxLayout *hBoxlayout = new QHBoxLayout;
  QWidget *widget = new QWidget;
  QWidget *subwidget = new QWidget;
  hBoxlayout->addWidget(input);
  hBoxlayout->addWidget(searchBtn);
  subwidget->setLayout(hBoxlayout);
  vBoxLayout->addWidget(subwidget);
  vBoxLayout->addWidget(table);
  widget->setLayout(vBoxLayout);
  setCentralWidget(widget);
  table->setShowGrid(true);
  table->setSelectionMode(QAbstractItemView::SingleSelection);
  table->setSelectionBehavior(QAbstractItemView::SelectRows);
  table->horizontalHeader()->setStretchLastSection(true);
  table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  model->setHorizontalHeaderItem(0, new QStandardItem(QObject::tr("Name")));
  model->setHorizontalHeaderItem(1, new QStandardItem(QObject::tr("Path")));
  table->setModel(model);
  delegate = new HTMLDelegate;
  table->setItemDelegate(delegate);
  connect(searchBtn, SIGNAL(released()), this, SLOT(search()));
  qRegisterMetaType<PUSN_RECORD>("Myclass");
  menu = menuBar()->addMenu("Build");
  buildIndex = new QAction(this);
  buildIndex->setText("Build Index");
  menu->addAction(buildIndex);
  connect(buildIndex, SIGNAL(triggered()), this, SLOT(buildIndexSlot()));

  pProgressBar = new QProgressBar();
  pLabel = new QLabel();
  pProgressBar->setRange(0, 100);
  pProgressBar->setValue(0);
  pProgressBar->setFixedWidth(0.4 * width());
  this->statusBar()->addWidget(pLabel);
  this->statusBar()->addPermanentWidget(pProgressBar);
}

void Something::initEngine() {
  pLabel->setText("Building Index......");
  searchBtn->setEnabled(false);
  int id = 0;
  for (auto ch : _drivers) {
    drivers.push_back(new USNParser(ch));
    monitors.push_back(new Monitor(id++, drivers.back()->root_handle, drivers.back()->journal));
    monitors.back()->start();
    connect(monitors.back(), SIGNAL(sendPUSN(int, PUSN_RECORD)), this, SLOT(recvPUSN(int, PUSN_RECORD)));
    fileindexs.push_back(new FileIndex(drivers.back()));
  }
  searcher = new Searcher(drivers, fileindexs, model);
  pLabel->setText("Finish");
  pProgressBar->setValue(100);
  searchBtn->setEnabled(true);
}

void Something::search() {
  auto query = input->text().toStdWString();
  searcher->parseQuery(query);
}

void Something::recvPUSN(int id, PUSN_RECORD pusn) {
  drivers[id]->recvPUSN(pusn);
}

Something::~Something() {
  delete ui;
}

void Something::closeEvent(QCloseEvent* e) {
  for (auto* ptr : monitors) ptr->terminate();
  e->accept();
}

void Something::buildIndexSlot() {
	QString path = QFileDialog::getExistingDirectory(this, tr("Choose folders"), ".");
  if (path.length() == 0) return;
	int id = 0;
	for (id = 0; id < _drivers.size(); ++id) {
		if (path[0] == _drivers[id]) break;
	}
	std::wstring temp = path.toStdWString();
	while (1) {
		int pos = temp.find(47);
		if (pos == -1)
			break;
		temp = temp.replace(pos, 1, L"\\");
	}
  const auto ref_num = drivers[id]->getFileRef(temp);
  if (ref_num == 0) return;
  std::set<FileEntry*> files;
  drivers[id]->recursiveAdd(ref_num, files);
  auto dataProcessor = new BuildIndexThread(std::move(files), drivers[id], fileindexs[id]);
  connect(dataProcessor, SIGNAL(setValue(int)), pProgressBar, SLOT(setValue(int)));
  connect(dataProcessor, SIGNAL(setLabel(QString)), pLabel, SLOT(setText(QString)));
  connect(dataProcessor, SIGNAL(enableBtn(bool)), searchBtn, SLOT(setEnabled(bool)));
  connect(dataProcessor, SIGNAL(enableBtn(bool)), buildIndex, SLOT(setEnabled(bool)));
  dataProcessor->start();
}
