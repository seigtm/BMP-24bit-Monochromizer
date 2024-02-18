/** @file bmp_24bit_monochromizer.cpp
 *  @brief BMP 24-bit monochromizer.
 *  @details This code converts a 24-bit BMP image to a 8-bit BMP image and makes it monochrome.
 *  @author Baranov Konstantin (seigtm) <gh@seig.ru>
 *  @version 1.0
 *  @date 2024-02-18
 */

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "Windows.h"

// Namespace with bmp related constants and file paths.
namespace setm::bmp {

static constexpr auto output_bmp_bit_count{ 8 };
static constexpr auto colors_in_palette{ 256 };

static const auto assets_directory{ std::filesystem::current_path().append("assets") };
static const auto input_bmp_file_path{ assets_directory / "input.bmp" };
static const auto output_bmp_file_path{ assets_directory / "output.bmp" };

};  // namespace setm::bmp

int main() {
    std::ifstream input_bmp_file{ setm::bmp::input_bmp_file_path, std::ios::binary };
    if(!input_bmp_file) {
        std::cerr << "Failed to open input file\n";
        return EXIT_FAILURE;
    }

    std::ofstream output_bmp_file{ setm::bmp::output_bmp_file_path, std::ios::binary };
    if(!output_bmp_file) {
        std::cerr << "Failed to create output file\n";
        return EXIT_FAILURE;
    }

    // Read bitmap headers.
    BITMAPFILEHEADER bmp_file_header;
    input_bmp_file.read(reinterpret_cast<char*>(&bmp_file_header), sizeof(BITMAPFILEHEADER));
    BITMAPINFOHEADER bmp_info_header;
    input_bmp_file.read(reinterpret_cast<char*>(&bmp_info_header), sizeof(BITMAPINFOHEADER));

    // Read pixel data from BMP file headers.
    const auto data_offset{ bmp_file_header.bfOffBits };
    const auto width{ bmp_info_header.biWidth };
    const auto height{ bmp_info_header.biHeight };

    // Reserve memory for pixel data.
    std::vector<RGBTRIPLE> input_buffer(width);
    std::vector<uint8_t> output_buffer(width);

    // Update headers.
    bmp_file_header.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 1024;
    bmp_info_header.biBitCount = setm::bmp::output_bmp_bit_count;
    bmp_file_header.bfSize = bmp_file_header.bfOffBits + width * height + height * (3 * width % 4);

    output_bmp_file.write(reinterpret_cast<const char*>(&bmp_file_header), sizeof(BITMAPFILEHEADER));
    output_bmp_file.write(reinterpret_cast<const char*>(&bmp_info_header), sizeof(BITMAPINFOHEADER));

    // Write palette.
    std::vector<RGBQUAD> palette(setm::bmp::colors_in_palette);
    for(auto& color : palette)
        color.rgbBlue = color.rgbGreen = color.rgbRed = static_cast<uint8_t>(&color - palette.data());
    output_bmp_file.write(reinterpret_cast<const char*>(palette.data()), palette.size() * sizeof(RGBQUAD));

    // Process pixel data.
    for(int i{}; i < height; ++i) {
        input_bmp_file.seekg(static_cast<std::streamoff>(data_offset) + i * width * sizeof(RGBTRIPLE));
        input_bmp_file.read(reinterpret_cast<char*>(input_buffer.data()), width * sizeof(RGBTRIPLE));

        for(int j{}; j < width; ++j) {
            const double luminance{ 0.3 * input_buffer[j].rgbtRed + 0.59 * input_buffer[j].rgbtGreen + 0.11 * input_buffer[j].rgbtBlue };
            output_buffer[j] = static_cast<uint8_t>(luminance);
        }

        output_bmp_file.write(reinterpret_cast<const char*>(output_buffer.data()), width * sizeof(uint8_t));

        // Padding.
        const std::size_t padding_size{ (4 - (width * sizeof(RGBTRIPLE)) % 4) % 4 };
        for(std::size_t k{}; k < padding_size; ++k)
            output_bmp_file.put(0);
    }

    std::cout << "Output 8-bit monochromized BMP file path: " << setm::bmp::output_bmp_file_path << '\n';
}
