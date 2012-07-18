#include <iostream>
#include <getopt.h>
#include <fstream>
#include <sstream>
#include <GL/gl.h>
#include <GL/glut.h>
#include "aruco.h"
using namespace cv;
using namespace aruco;

string TheInputVideo;
string TheIntrinsicFile;
bool The3DInfoAvailable=false;
float TheMarkerSize=-1;
ArMarkerDetector PPDetector;
VideoCapture TheVideoCapturer;
vector<Marker> TheMarkers;
Mat TheInputImage,TheUndInputImage;
Mat TheIntriscCameraMatrix,TheDistorsionCameraParams,TheResizedImage;
Size TheGlWindowSize;
bool TheCaptureFlag=true;
bool readIntrinsicFile(string TheIntrinsicFile,Mat & TheIntriscCameraMatrix,Mat &TheDistorsionCameraParams,Size size);
void readArguments ( int argc,char **argv );
void usage();
void vDrawScene();
void vIdle();
void vResize( GLsizei iWidth, GLsizei iHeight );
void vMouse(int b,int s,int x,int y);
/************************************
 *
 *
 *
 *
 ************************************/

int main(int argc,char **argv)
{
	try
	{
		readArguments (argc,argv);
		if (TheIntrinsicFile==""){cerr<<"-f option required"<<endl;return -1;}
		if (TheMarkerSize==-1){cerr<<"-s option required"<<endl;return -1;}
								 //read from camera
		if (TheInputVideo=="") TheVideoCapturer.open(0);
		else TheVideoCapturer.open(TheInputVideo);
		if (!TheVideoCapturer.isOpened())
		{
			cerr<<"Could not open video"<<endl;
			return -1;

		}

		//read first image
		TheVideoCapturer>>TheInputImage;
		//read camera paramters if passed
		if (!readIntrinsicFile(TheIntrinsicFile,TheIntriscCameraMatrix,TheDistorsionCameraParams,TheInputImage.size()))
		{
			cerr<<"could not open file "<<TheIntrinsicFile<<endl;
			return -1;

		}

		glutInit(&argc, argv);
		glutInitWindowPosition( 0, 0);
		glutInitWindowSize(TheInputImage.size().width,TheInputImage.size().height);
		glutInitDisplayMode( GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE );
		glutCreateWindow( "AruCo" );
		glutDisplayFunc( vDrawScene );
		glutIdleFunc( vIdle );
		glutReshapeFunc( vResize );
		 glutMouseFunc(vMouse);
		glClearColor( 0.0, 0.0, 0.0, 1.0 );
		glClearDepth( 1.0 );
		TheGlWindowSize=TheInputImage.size();
		vResize(TheGlWindowSize.width,TheGlWindowSize.height);
		glutMainLoop();

	}catch(std::exception &ex)

	{
		cout<<"Exception :"<<ex.what()<<endl;
	}

}
/************************************
 *
 *
 *
 *
 ************************************/

void vMouse(int b,int s,int x,int y)
{
    if (b==GLUT_LEFT_BUTTON && s==GLUT_DOWN) {
      TheCaptureFlag=!TheCaptureFlag;
    }

}

/************************************
 *
 *
 *
 *
 ************************************/
void axis(float size)
{
    glColor3f (1,0,0 );
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f); // origin of the line
    glVertex3f(0.0f, size, 0.0f); // ending point of the line
    glEnd( );

    glColor3f ( 0,1,0 );
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f); // origin of the line
    glVertex3f(size, 0.0f, 0.0f); // ending point of the line
    glEnd( );


    glColor3f (0,0,1 );
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f); // origin of the line
    glVertex3f(0.0f, 0.0f, size); // ending point of the line
    glEnd( );

 

}
/************************************
 *
 *
 *
 *
 ************************************/
void vDrawScene()
{
	///clear
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	///draw image in the buffer
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, TheGlWindowSize.width, 0, TheGlWindowSize.height, -1.0, 1.0);
	glViewport(0, 0, TheGlWindowSize.width , TheGlWindowSize.height);
	glDisable(GL_TEXTURE_2D);
	glPixelZoom( 1, -1);
	glRasterPos3f( 0, TheGlWindowSize.height  - 0.5, -1.0 );
	glDrawPixels ( TheGlWindowSize.width , TheGlWindowSize.height , GL_BGR , GL_UNSIGNED_BYTE , TheResizedImage.ptr(0) );
	///Set the appropriate projection matrix so that rendering is done in a enrvironment
	//like the real camera (without distorsion)
	glMatrixMode(GL_PROJECTION);
	double proj_matrix[16];
	ArMarkerDetector::glGetProjectionMatrix(TheIntriscCameraMatrix,TheInputImage.size(),TheGlWindowSize,proj_matrix,0.05,10);
	glLoadIdentity();
	glLoadMatrixd(proj_matrix);

	//now, for each marker, 
	double modelview_matrix[16];
	for(unsigned int m=0;m<TheMarkers.size();m++)
	{
		TheMarkers[m].glGetModelViewMatrix(modelview_matrix);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glLoadMatrixd(modelview_matrix);	
		
				
//		axis(TheMarkerSize);

		glColor3f(1,0.4,0.4);
		glTranslatef(TheMarkerSize/2, TheMarkerSize/2,TheMarkerSize/2);
		glPushMatrix();
 		glutWireCube( TheMarkerSize );

		glPopMatrix();
	}

	glutSwapBuffers();

}


/************************************
 *
 *
 *
 *
 ************************************/
void vIdle()
{
  if(TheCaptureFlag){
	//capture image
	TheVideoCapturer.grab();
	TheVideoCapturer.retrieve( TheInputImage);
	TheUndInputImage.create(TheInputImage.size(),CV_8UC3);
	//remove distorion in image
	cv::undistort(TheInputImage,TheUndInputImage, TheIntriscCameraMatrix, TheDistorsionCameraParams);
	//detect markers
	PPDetector.detect(TheUndInputImage,TheMarkers,TheIntriscCameraMatrix,Mat(),TheMarkerSize);
	//resize the image to the size of the GL window
	cv::resize(TheUndInputImage,TheResizedImage,TheGlWindowSize);	
  }
  glutPostRedisplay();
}


/************************************
 *
 *
 *
 *
 ************************************/
void vResize( GLsizei iWidth, GLsizei iHeight )
{
	TheGlWindowSize=Size(iWidth,iHeight);
}


/************************************
 *
 *
 *
 *
 ************************************/
void usage()
{
	cout<<"This program allows \n\n";
}


/************************************
 *
 *
 *
 *
 ************************************/
static const char short_options [] = "hi:f:s:";

static const struct option
long_options [] =
{
	{ "help",           no_argument,   NULL,                 'h' },
	{ "input",     required_argument,   NULL,           'i' },
	{ "intFile",     required_argument,   NULL,           'f' },
	{ "size",     required_argument,   NULL,           's' },

	{ 0, 0, 0, 0 }
};

/************************************
 *
 *
 *
 *
 ************************************/
void readArguments ( int argc,char **argv )
{
	for ( ;; )
	{
		int index;
		int c;
		c = getopt_long ( argc, argv,
			short_options, long_options,
			&index );

		if ( -1 == c )
			break;
		switch ( c )
		{
			case 0:
				break;
			case 'h':
				usage ();
				exit ( EXIT_SUCCESS );
				break;
			case 'i':
				TheInputVideo=optarg;
				break;
			case 'f':
				TheIntrinsicFile=optarg;
				break;
			case 's':
				TheMarkerSize=atof(optarg);
				break;
			default:
				usage ();
				exit ( EXIT_FAILURE );
		};
	}

}


/************************************
 *
 *
 *
 *
 ************************************/
bool readIntrinsicFile(string TheIntrinsicFile,Mat & TheIntriscCameraMatrix,Mat &TheDistorsionCameraParams,Size size)
{
	ifstream InFile(TheIntrinsicFile.c_str());
	if (!InFile) return false;
	char line[1024];
	InFile.getline(line,1024);	 //skype first
	InFile.getline(line,1024);
	stringstream InLine;
	InLine<<line;
	TheDistorsionCameraParams.create(4,1,CV_32FC1);

	TheIntriscCameraMatrix=Mat::eye(3,3,CV_32FC1);

								 //fx
	InLine>>TheIntriscCameraMatrix.at<float>(0,0);
								 //fy
	InLine>>TheIntriscCameraMatrix.at<float>(1,1);
								 //cx
	InLine>>TheIntriscCameraMatrix.at<float>(0,2);
								 //cy
	InLine>>TheIntriscCameraMatrix.at<float>(1,2);
	for(int i=0;i<4;i++)
		InLine>>TheDistorsionCameraParams.at<float>(i,0);
	//now, read the camera size
	float width,height;
	InLine>>width>>height;
	//resize the camera parameters to fir this image size
	float AxFactor= float(size.width)/ width;
	float AyFactor= float(size.height)/ height;
	TheIntriscCameraMatrix.at<float>(0,0)*=AxFactor;
	TheIntriscCameraMatrix.at<float>(0,2)*=AxFactor;
	TheIntriscCameraMatrix.at<float>(1,1)*=AyFactor;
	TheIntriscCameraMatrix.at<float>(1,2)*=AyFactor;

	//debug
	cout<<"fx="<<TheIntriscCameraMatrix.at<float>(0,0)<<endl;
	cout<<"fy="<<TheIntriscCameraMatrix.at<float>(1,1)<<endl;
	cout<<"cx="<<TheIntriscCameraMatrix.at<float>(0,2)<<endl;
	cout<<"cy="<<TheIntriscCameraMatrix.at<float>(1,2)<<endl;
	cout<<"k1="<<TheDistorsionCameraParams.at<float>(0,0)<<endl;
	cout<<"k2="<<TheDistorsionCameraParams.at<float>(0,1)<<endl;
	cout<<"p1="<<TheDistorsionCameraParams.at<float>(0,2)<<endl;
	cout<<"p2="<<TheDistorsionCameraParams.at<float>(0,3)<<endl;

	return true;
}


/*

void argDispImageDrawPixels( unsigned char *image, int xwin=0, int ywin=0 )
{
	float    sx, sy;
	GLfloat  zoom;

	if( xwin == 0 && ywin == 0 ) {
	zoom = 1;
		sx = 0;
		sy = gWinYsize - 0.5;
	}
	else if( xwin == 1 && ywin == 0 ) {
	zoom = 1;
		sx = gXsize;
		sy = gWinYsize - 0.5;
	}
	else {
		zoom = gZoom / (double)GMINI;
		sx = (xwin-1)*gMiniXsize;
		sy = gWinYsize - gYsize - (ywin-1)*gMiniYsize - 0.5;
	}
	glDisable(GL_TEXTURE_2D);
	glPixelZoom( zoom, -zoom);
	glRasterPos3f( sx, sy, -1.0 );

	glDrawPixels( gImXsize, gImYsize, GL_BGR, GL_UNSIGNED_BYTE, image );

}*/
