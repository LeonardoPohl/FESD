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
		std::cout << 100 * ((float)irec++ / (float)recordings.size()) << "% Done!" << std::endl;
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
		}
	}
}