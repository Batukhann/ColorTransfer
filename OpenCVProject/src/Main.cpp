#include <iostream>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

// for Image Statistics
struct ImageStat {
	cv::Mat channels[3];
	cv::Scalar mean[3], stdev[3];
};

// for memory checking
struct AllocataionMetrics {
	uint32_t TotalAlocated = 0;
	uint32_t TotalFreed = 0;
	uint32_t CurrentUsage() { return TotalAlocated - TotalFreed; }
};

static AllocataionMetrics allocataionMetrics;

// overload new and delete operation for counting memory allocation
void* operator new(size_t size) {
	allocataionMetrics.TotalAlocated += size;
	return malloc(size);
}

void operator delete(void* memory, size_t size) {
	allocataionMetrics.TotalFreed += size;
	free(memory);
}

// print memory usage at that point
void PrintMemoryUsage() {
	std::cout << "Current Memory Usage: " << allocataionMetrics.CurrentUsage() << " bytes" << std::endl;
}


// read image from given path and return that image
cv::Mat ReadImage(const std::string &imagePath) {

	cv::Mat image = cv::imread(imagePath, cv::IMREAD_COLOR);

	if (image.empty()) {
		std::cout << "Could not read the image. Terminating code" << std::endl;
		exit(1);
	}

	std::cout << imagePath << " succesfully read" << std::endl;
	std::cout << "width: " << image.size().width << std::endl;
	std::cout << "height: " << image.size().height << std::endl;
	std::cout << "channels: " << image.channels() << std::endl;

	return image;
}

// calculates means and standart dev from given image using opencv
ImageStat CalculateMeanStdDev(const cv::Mat& imageLab) {

	ImageStat imgStat;

	cv::split(imageLab, imgStat.channels);
	
	cv::meanStdDev(imgStat.channels[0], imgStat.mean[0], imgStat.stdev[0]);
	cv::meanStdDev(imgStat.channels[1], imgStat.mean[1], imgStat.stdev[1]);
	cv::meanStdDev(imgStat.channels[2], imgStat.mean[2], imgStat.stdev[2]);

	return imgStat;

}


void ManipulateChannels(ImageStat& source, ImageStat& target) {

	target.channels[0] -= target.mean[0];
	target.channels[1] -= target.mean[1];
	target.channels[2] -= target.mean[2];

	double primeL = target.stdev[0](0) / source.stdev[0](0);
	double primeA = target.stdev[1](0) / source.stdev[1](0);
	double primeB = target.stdev[2](0) / source.stdev[2](0);

	target.channels[0] *= primeL;
	target.channels[1] *= primeA;
	target.channels[2] *= primeB;

	target.channels[0] += source.mean[0];
	target.channels[1] += source.mean[1];
	target.channels[2] += source.mean[2];

}


cv::Mat ColorTransfer(const cv::Mat& source, const cv::Mat& target) {

	// OpenCV Matrix for LAB space
	cv::Mat sourceLab;
	cv::Mat targetLab;
	// OpenCV Matrix for result
	cv::Mat result;

	// converting BGR image to LAB image
	cv::cvtColor(source, sourceLab, cv::COLOR_BGR2Lab);
	cv::cvtColor(target, targetLab, cv::COLOR_BGR2Lab);

	sourceLab.convertTo(sourceLab, CV_32F);
	targetLab.convertTo(targetLab, CV_32F);

	// Calculate mean and standart dev for both lab image
	ImageStat sourceStat = CalculateMeanStdDev(sourceLab);
	ImageStat targetStat = CalculateMeanStdDev(targetLab);

	ManipulateChannels(sourceStat, targetStat);

	// merge target channels
	cv::merge(targetStat.channels, 3, result);
	result.convertTo(result, CV_8U);
	// return BGR space from LAB space
	cv::cvtColor(result, result, cv::COLOR_Lab2BGR);

	return result;

}


int main(int argc, char* argv[]) {

	{
		// define the source and target image paths
		std::string sourcePath = "images/castle.jpg";
		std::string targetPath = "images/comp.jpg";

		// read images from given path
		cv::Mat source = ReadImage(sourcePath);
		cv::Mat target = ReadImage(targetPath);

		// color transfer
		cv::Mat result = ColorTransfer(source, target);

		// combine target source and results to one image
		cv::hconcat(target, source, target);
		cv::hconcat(target, result, result);
		cv::resize(result, result, cv::Size(), 0.5, 0.5);

		// write the result to the disk 
		cv::imwrite("result.png", result);

		std::cout << "Color Transformation completed and result save to the result.png " << std::endl;;

		// check memory before the end of the scope
		PrintMemoryUsage();
	}
	// check memory again for any leaks 
	PrintMemoryUsage(); // 0 memory leak confirmed
	return 0;
}