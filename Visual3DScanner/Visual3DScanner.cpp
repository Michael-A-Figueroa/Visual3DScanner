// Visual3DScanner.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "DataFile.h"
#include "DataStructs.h"
#include "opencv2/xfeatures2d.hpp"
#include "opencv2/calib3d.hpp"
#include "opencv2/viz.hpp"

cv::Point2d getPoint(cv::KeyPoint keypoint){return keypoint.pt;}

void displayPntCloud(std::vector<cv::Point3d> pointCloud)
{
	cv::viz::Viz3d myWindow("Window");

	cv::viz::WCloud cloud(pointCloud);

	myWindow.showWidget("bunny",cloud);

	myWindow.spin();
}

int main()
{
	const int maxDescriptors = 50;

	DataFile image_data{"D:\\Data\\dino multiview dataset","dino_par.txt"};
	std::vector<ImageStruct> images;
	auto sift = cv::xfeatures2d::SIFT::create(25);

	//local variables to store data
	cv::Mat lastImage;
	cv::Mat currentImage;
	cv::Mat lastProjMat;
	cv::Mat currentProjMat;
	std::vector<cv::Point3d> _3Dpoints;


	for (int i = 0; i < image_data.getDataSize(); ++i)
	{
		std::cout << "calculating 3D points at image index: " << i << std::endl;
		auto data = image_data.getNext(i); // first = image, second = projection matrix

		currentImage = data.first;
		currentProjMat = data.second;

		if (lastImage.empty())
		{
			lastImage = currentImage;
			lastProjMat = currentProjMat;
			continue;
		}

		// use sift to detect keypoints and features
		std::vector<cv::KeyPoint> lastImgKeyPts;
		std::vector<cv::KeyPoint> currentImgKeyPts;
		//detect and compute last image keypoints and descriptors
		sift->detect(lastImage, lastImgKeyPts);
		//detect and compute current image keypoints and descriptors
		sift->detect(currentImage, currentImgKeyPts);
		// triangulate points
		cv::Mat homog4DPoints;
		std::vector<cv::Point2d> lastpts(lastImgKeyPts.size());
		std::vector<cv::Point2d> currentpts(currentImgKeyPts.size());
		std::transform(lastImgKeyPts.begin(), lastImgKeyPts.end(), lastpts.begin(), getPoint);
		std::transform(currentImgKeyPts.begin(), currentImgKeyPts.end(), currentpts.begin(), getPoint);
		
		const auto currentPtsLarger = currentpts.size() > lastpts.size() ? true : false;
		while (lastpts.size() != currentpts.size())
		{
			if (currentPtsLarger)
			{
				currentpts.pop_back();
			}
			else
			{
				lastpts.pop_back();
			}
		}


		cv::triangulatePoints(lastProjMat, currentProjMat, lastpts, currentpts, homog4DPoints);

		for (int i = 0; i < homog4DPoints.cols; ++i)
		{
			cv::Mat nonHomoPt(4,1,CV_64F);
			cv::Mat col = homog4DPoints.col(i).clone();
			col = col.reshape(4);
			cv::convertPointsFromHomogeneous(col, nonHomoPt);
			//std::cout << nonHomoPt << std::endl;
			_3Dpoints.push_back(cv::Point3d(nonHomoPt.at<cv::Vec3d>(0)[0], nonHomoPt.at<cv::Vec3d>(0)[1], nonHomoPt.at<cv::Vec3d>(0)[2]));
		}
	}

	displayPntCloud(_3Dpoints);
    return 0;
}

