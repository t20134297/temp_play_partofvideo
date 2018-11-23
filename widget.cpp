#include "widget.h"
#include "ui_widget.h"
#include<QMouseEvent>
#include<opencv2/opencv.hpp>
#include<QFileDialog>
#include "QDebug"
#include<unistd.h>
#include<ctime>
#include <QMessageBox>
#include<QPainter>
#include<mysql/mysql.h>


using namespace cv;
using namespace std;



Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    connect(ui->pushButton,SIGNAL(clicked()),this,SLOT(select_video()) );
    connect(ui->pushButton_2,SIGNAL(clicked()),this,SLOT(play_or_pause()) );
    connect(ui->pushButton_3,SIGNAL(clicked()),this,SLOT(speedup_play()) );
    connect(ui->pushButton_5,SIGNAL(clicked()),this,SLOT(slowdown_play()) );
    connect(ui->horizontalSlider,SIGNAL(sliderReleased()),this,SLOT(change_play_process())  );
    connect(ui->horizontalSlider,SIGNAL(sliderPressed()),this,SLOT(press_slider())  );
    connect(ui->pushButton_4,SIGNAL(clicked()),this,SLOT(capture_image()) );
    connect(ui->pushButton_6,SIGNAL(clicked()),this,SLOT(search_ped()) );
}

Widget::~Widget()
{
    delete ui;
}

void Widget::select_video()
{
    int height = ui->label->height();
    int width = ui->label->width();
    Mat backgroundimg;
    QImage img;
    QPixmap qpix;
    QFileDialog *fileDialog = new QFileDialog(this);
    fileDialog->setWindowTitle("open video");
    fileDialog->setDirectory("/home/ansheng/Desktop/hikvision");
    fileDialog->setNameFilter(tr("(*.mp4 *.mkv)"));
    fileDialog->setViewMode(QFileDialog::Detail);
    QStringList fileName;
    if(fileDialog->exec())
      {
         fileName = fileDialog->selectedFiles();
      }
    videoname = fileName.at(0).toStdString();
    name = videoname;
    cap = VideoCapture(videoname);
    cap>>play_frame;
    backgroundimg = play_frame;
    cvtColor(backgroundimg,backgroundimg,CV_BGR2RGB);
    img = QImage((const unsigned char*)(backgroundimg.data),backgroundimg.cols,backgroundimg.rows,backgroundimg.cols*backgroundimg.channels(),QImage::Format_RGB888);
    qpix = QPixmap::fromImage(img);
    qpix = qpix.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);  //按比例填充
    ui->label->setPixmap(qpix);
    frame_length = cap.get(CV_CAP_PROP_FRAME_COUNT);
    frame_position = 1;
    ui->horizontalSlider->setMaximum(frame_length-3);
    ui->horizontalSlider->setMinimum(1);
    ui->horizontalSlider->setValue(frame_position);
}



void Widget::play_or_pause()
{
    playorpause = !playorpause;
    QImage img;
    QPixmap qpix;
    int height = ui->label->height();
    int width = ui->label->width();
//    ui->label->resize(1280,720);

    while(play_frame.data && playorpause)
    {
        frame_position += 1;
        ui->label_2->setText(QString::number(frame_position, 10));
        ui->horizontalSlider->setValue(frame_position);
        cap>>play_frame;
        cvtColor(play_frame,play_frame,CV_BGR2RGB);
        img = QImage((const unsigned char*)(play_frame.data),play_frame.cols,play_frame.rows,play_frame.cols*play_frame.channels(),QImage::Format_RGB888);
        qpix = QPixmap::fromImage(img);
        qpix = qpix.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);  //按比例填充
        ui->label->setPixmap(qpix);
        usleep(waittime);
        waitKey(5);
    }
}

void Widget::speedup_play()
{
    if(speed<10)
    {
        speed = speed+1;
        waittime = waittime/speed;
    }
}

void Widget::slowdown_play()
{
     waittime = waittime + 3000;
}

void Widget::press_slider()
{
    playorpause = !playorpause;
}

void Widget::change_play_process()
{
    frame_position = ui->horizontalSlider->value();
    cap.set(CV_CAP_PROP_POS_FRAMES,frame_position);
    ui->pushButton_2->click();
}

void Widget::capture_image()
{
    Mat captured;
    captured = play_frame;
    int height = ui->label->height();
    int width = ui->label->width();
    QImage img;
    QPixmap qpix;
//    cvtColor( play_frame, play_frame,CV_BGR2RGB);
    img = QImage((const unsigned char*)( play_frame.data), play_frame.cols, play_frame.rows, play_frame.cols* play_frame.channels(),QImage::Format_RGB888);
    qpix = QPixmap::fromImage(img);
    qpix = qpix.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);  //按比例填充
    ui->label_4->setPixmap(qpix);

    QMessageBox::information(this, QString::fromLocal8Bit("information"),QString::fromLocal8Bit("imageSaved"));

}

void Widget::search_ped()
{
    string cameraid = videoname.substr(videoname.find_last_of("/")+1,videoname.find(".")-videoname.find_last_of("/")-1);
    vector<string> temp_result;
    vector<string> final_result;
    string sql = "select * from ReId where start<";
    int position = frame_position;
    sql = sql+to_string(position) +" and end > " +to_string(position)+" and cameraId = "+"'"+cameraid+"'"+" ;";
    cout<<sql<<endl;
    MYSQL mysql;
    mysql_init(&mysql);
    mysql_real_connect(&mysql,"localhost","root","neuadmin","first",3306,NULL,0);

    mysql_query(&mysql,sql.c_str());
    MYSQL_RES *result = NULL;
    result = mysql_store_result(&mysql);

    MYSQL_ROW row = NULL;
    row = mysql_fetch_row(result);
    while(NULL != row)
    {
        for(int i=1;i<6;i=i+1)
        {
//            cout<<"in get_record_from_database function:  "<<row[i]<<endl;
            temp_result.push_back(row[i]);
        }
        row = mysql_fetch_row(result);
    }

    mysql_free_result(result);
    mysql_close(&mysql);

    final_result.push_back(temp_result[0]);
    bool readd=false;
    for(int i=1;i<temp_result.size();i=i+1)
    {
        readd=false;
        for(int j=0;j<final_result.size();j=j+1)
        {
            if(temp_result[i] == final_result[j])
            {
                readd = true;
            }
        }
        if (!readd)
        {
            final_result.push_back(temp_result[i]);
        }
    }
//    for(int i=0;i<final_result.size();i=i+1)
//    {
//        cout<<final_result[i]<<endl;
//    }
    top_choosed_videos = final_result;
    for(int i=0;i<top_choosed_videos.size();i=i+1)
    {
        cout<<top_choosed_videos[i]<<endl;
    }
    num_choosed_videos = final_result.size();

    string path_with_slash = videoname.substr(0,videoname.find_last_of("/")+1); //for exmple /home/ansheng/desktop/      with the last slash
    cout<<path_with_slash<<endl;

    string temp_top = top_choosed_videos[0];
    string caid = temp_top.substr(0,temp_top.find("_"));
    string pos = temp_top.substr(temp_top.find("_ps")+3,temp_top.length());
    string p1,p2;
    p1 = pos.substr(0,pos.find("_"));
    p2 = pos.substr(pos.find("_pe")+3,pos.find(".")-pos.find("_pe")-3);
    cout<<caid<<endl;
    cout<<p1<<endl;
    cout<<p2<<endl;

    string playvideo = path_with_slash + caid+".mp4";
    int start = stoi(p1);
    int end = stoi(p2);

}









//you are beautiful
