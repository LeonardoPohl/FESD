#pragma once
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdio>

#include <opencv2/opencv.hpp>
#include <json/json.h>

#include "Consts.h"

void convertRecordings(std::vector<Json::Value> recordings, bool delete_old = false)  {
	int irec = 0;

	for (auto rec : recordings) {
		std::cout << 100 * ((float)irec++ / (float)recordings.size()) << "% Done!                                " << std::endl;
		std::filesystem::path frames = m_RecordingDirectory / std::filesystem::path{ rec["Cameras"][0]["FileName"].asString() };
		int iframe = 0;
		for (const auto& entry : std::filesystem::directory_iterator(frames))
		{
			if (entry.path().extension() == ".yml") {
				std::cout << iframe++ << "/" << rec["Frames"].asInt() << " Frames stored!\r";
				cv::FileStorage frameStore((entry.path()).string(), cv::FileStorage::READ);

				cv::Mat frame;
				frameStore["frame"] >> frame;
				frameStore.release();

				auto bin_path = entry.path();
				bin_path.replace_extension(".bin");
				std::ofstream fs(bin_path, std::fstream::binary);

				// Header
				int type = frame.type();
				int channels = frame.channels();
				fs.write((char*)&frame.rows, sizeof(int));		// rows
				fs.write((char*)&frame.cols, sizeof(int));		// cols
				fs.write((char*)&type, sizeof(int));			// type
				fs.write((char*)&channels, sizeof(int));    // channels

				// Data
				if (frame.isContinuous())
				{
					fs.write(frame.ptr<char>(0), (frame.dataend - frame.datastart));
				}
				else
				{
					int rowsz = CV_ELEM_SIZE(type) * frame.cols;
					for (int r = 0; r < frame.rows; ++r)
					{
						fs.write(frame.ptr<char>(r), rowsz);
					}
				}

				if (delete_old) {
					std::remove(entry.path().string().c_str());
				}
			}
			else if (entry.path().extension() == ".bin") {
				std::cout << iframe++ << "/" << rec["Frames"].asInt() << " Frames stored!\r";
				std::ifstream fin(entry.path(), std::fstream::binary);

				// Header
				int rows, cols, type, channels;
				fin.read((char*)&rows, sizeof(int));         // rows
				fin.read((char*)&cols, sizeof(int));         // cols
				fin.read((char*)&type, sizeof(int));         // type
				fin.read((char*)&channels, sizeof(int));     // channels
				if (type == CV_16F) {
					std::cout << "Already converted!" << std::endl;
					continue;
				}
				// Data
				cv::Mat mat(rows, cols, type);
				fin.read((char*)mat.data, CV_ELEM_SIZE(type) * rows * cols);
				fin.close();
				std::remove(entry.path().string().c_str());
				mat.convertTo(mat, CV_16F);

				if (delete_old) {
					std::remove(entry.path().string().c_str());
				}

				auto bin_path = entry.path();
				bin_path.replace_extension(".bin");
				std::ofstream fout(bin_path, std::fstream::binary);

				// Header
				type = mat.type();
				channels = mat.channels();
				fout.write((char*)&mat.rows, sizeof(int));		// rows
				fout.write((char*)&mat.cols, sizeof(int));		// cols
				fout.write((char*)&type, sizeof(int));			// type
				fout.write((char*)&channels, sizeof(int));    // channels

				// Data
				if (mat.isContinuous())
				{
					fout.write(mat.ptr<char>(0), (mat.dataend - mat.datastart));
				}
				else
				{
					int rowsz = CV_ELEM_SIZE(type) * mat.cols;
					for (int r = 0; r < mat.rows; ++r)
					{
						fout.write(mat.ptr<char>(r), rowsz);
					}
				}
			}
		}
	}
}