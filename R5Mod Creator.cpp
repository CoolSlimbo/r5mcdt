#include "spdlog/spdlog.h"
#include "spdlog/stopwatch.h"
#include "color.hpp"
#include "cryptopp/sha.h"
#include "cryptopp/zdeflate.h"
#include "cryptopp/zinflate.h"
#include "cryptopp/hex.h"
#include <iostream>
#include <fstream>
#include <regex>
#include <filesystem>

using namespace std;

int selectedOption;
void o_selector();
int main();

// Function to read a string - strings are done in the following format:
// 2 bytes - 16 bit unsigned integer - length of string
// n bytes - string data
// Input: *ifstream - file to read from
char* readString(ifstream* file) {
	// Read the length of the string
	uint16_t length;
	file->read((char*)&length, 2);

	// Read the string data
	char* data = new char[length];
	file->read(data, length);

	data[length] = '\0';

	// Return the data
	return data;
}

// Function to write a string - strings are done in the following format:
// 2 bytes - 16 bit unsigned integer - length of string
// n bytes - string data
// Input: *ofstream - file to write to
//		  string - string to write - automagically trims null terminating character
void writeString(ofstream* file, string str) {
	// Get the length
	unsigned int length = str.length();
	// Write the length
	file->write(reinterpret_cast<const char*>(&length), 2);
	// Write the string
	file->write(str.c_str(), length);
}

void buildMod(string disName, string intName, string fileName, string desc, string ver, string sdkVer, string folder, string authors) {
	spdlog::info("Building mod...");

	ofstream file(fileName + ".r5mod", ios::binary);
	
	// Write the text "R5R" to the file
	file.write(reinterpret_cast<const char*>("R5R"), 3);
	
	// Write the SDK version
	writeString(&file, sdkVer);

	// Save the position of the hash so it can be written once hash is computed
	int hashPos = file.tellp();
	// Write 20 bytes of 0s for the hash
	file.write(reinterpret_cast<const char*>("00000000000000000000"), 20);
	// Write 16 bytes of ?s for the appid
	file.write(reinterpret_cast<const char*>("????????????????"), 16);

	// Write the display name
	writeString(&file, disName);
	// Write the internal name
	writeString(&file, intName);
	// Write the version
	writeString(&file, ver);
	// Write the description
	writeString(&file, desc);
	// Write the author
	writeString(&file, authors);

	// Write the dependancy count - 0 because I'm to lazy to impliment
	file.write(reinterpret_cast<const char*>("0"), 1);
	// Since I'm to lazy to add deps, the deps add function isn't here

	// Save the position of the filecount, so it, and bytes remanining can be written later
	int fileCountPos = file.tellp();
	// Write 4 bytes of 0s for the file count
	file.write(reinterpret_cast<const char*>("0000"), 4);
	// Write 4 bytes of 0s for the bytes remaining
	file.write(reinterpret_cast<const char*>("0000"), 4);

	CryptoPP::SHA1 shaHash;

	int fileCount = 0;
	int bytesRemaining = 0;

	// For each file in the folder, check if it's a directory/empty, if it's not, then right the mod
	for (auto& e : filesystem::recursive_directory_iterator(folder)) {
		if (e.is_directory()) continue;
		if (e.path().extension() == "exe" || e.path().extension() == "r5mod") continue;

		// The file structure consists of a string with the location of the file, the uncompressed size as a 32-bit unsigned int, the compressed size as a 32-bit unsigned int, and the compressed file data.

		// Write the file path, minus the folder name
		writeString(&file, e.path().string().substr(folder.length() + 1));
		
		// Write the uncompressed size
		uint32_t uncompressedSize = e.file_size();
		file.write(reinterpret_cast<const char*>(&uncompressedSize), 4);
		// Save the position of the compressed size, so it can be written later
		int compressedSizePos = file.tellp();
		// Write 4 bytes of 0s for the compressed size
		file.write(reinterpret_cast<const char*>("0000"), 4);
		
		// Open the file and write it to a buffer
		char* data = new char[uncompressedSize];
		
		ifstream fileToCompress(e.path().string(), ios::binary);
		fileToCompress.read(data, uncompressedSize);
		fileToCompress.close();

		// Compress the file
		string compressedData;
		
		CryptoPP::Deflator deflator(new CryptoPP::StringSink(compressedData));
		deflator.Put((const CryptoPP::byte*)data, uncompressedSize);
		deflator.MessageEnd();

		file.seekp(compressedSizePos);
		int compressedSize = compressedData.length();
		file.write(reinterpret_cast<const char*>(&compressedSize), 4);
		file.seekp(0, ios::end);

		shaHash.Update((const CryptoPP::byte*)compressedData.c_str(), compressedSize);

		file.write(compressedData.c_str(), compressedSize);

		bytesRemaining += e.path().string().length() + 10 + compressedSize;
		fileCount++;
	}

	string digest;
	digest.resize(shaHash.DigestSize() / 2);
	// Data being hashed is the compressed data
	shaHash.TruncatedFinal((CryptoPP::byte*)&digest[0], digest.size());

	string hashString;
	CryptoPP::HexEncoder encoder(new CryptoPP::StringSink(hashString));
	CryptoPP::StringSource(digest, true, new CryptoPP::Redirector(encoder));

	file.seekp(hashPos);
	file.write(hashString.c_str(), 20);

	file.seekp(fileCountPos);
	file.write(reinterpret_cast<const char*>(&fileCount), 4);
	file.write(reinterpret_cast<const char*>(&bytesRemaining), 4);

	spdlog::info("Mod has been succefully been built!");
	cout << dye::purple("File Count: ") << fileCount << endl
		<< dye::purple("File Name: ") << fileName << dye::aqua(".r5mod") << endl;
}


void o_CreateMod() {
	// Asks the user for information about the mod and creates a mod file
	spdlog::info("Welcome to the mod creator!");
	spdlog::info("Please follow the prompts to build your mod file.");
	spdlog::warn("Fields marked with an asterisk (*) are required.");
displayName:
	cout << "Mod Display Name(" << dye::red("*") << "): ";
	string displayName = "";
	getline(cin, displayName);
	if (displayName == "") {
		spdlog::error("Display name cannot be empty!");
		goto displayName;
	}
	
internalName:
	cout << "Internal Mod Name - Can not contain spaces(" << dye::red("*") << "): ";
	string internalName = "";
	getline(cin, internalName);
	if (internalName == "") {
		spdlog::error("Internal name cannot be empty!");
		goto internalName;
	}
	if (regex_match(internalName, regex(".*\\s.*"))) {
		spdlog::error("Internal name cannot contain spaces!");
		goto internalName;
	}
	
sdkVersion:
	cout << "SDK Version(" << dye::red("*") << "): ";
	string sdkVersion = "";
	getline(cin, sdkVersion);
	if (sdkVersion == "") {
		spdlog::error("SDK version cannot be empty!");
		goto sdkVersion;
	}
	
	cout << "Mod Description: ";
	string description = "";
	getline(cin, description);
	
	cout << "Mod Version: ";
	string version = "";
	getline(cin, version);

	cout << "Mod Author: ";
	string author = "";
	getline(cin, author);

	// Find the folder being used to convert into a mod
	spdlog::warn("Folder is relative to the executable!");
folder:
	string folder = "";
	cout << "Folder name to convert into a mod(" << dye::red("*") << "): ";
	getline(cin, folder);
	if (folder == "") {
		spdlog::error("Folder name cannot be empty!");
		goto folder;
	}
	if (!filesystem::exists(folder)) {
		spdlog::error("Folder does not exist!");
		goto folder;
	}
	if (filesystem::is_empty(folder)) {
		spdlog::error("Folder is empty!");
		goto folder;
	}
	
fileName:
	string fileName = internalName;
	cout << "File name of mod (leave blank to use internal name): ";
	getline(cin, fileName);
	// If filename is blank, set it to internalName
	if (fileName == "")
		fileName = internalName;
	if (filesystem::exists(fileName + ".r5mod")) {
		spdlog::warn("File of same name already exists in current directory.");
		cout << "Would you like to override this file? (y/n): ";
		string response;
		getline(cin, response);
		if (response == "n")
			goto fileName;
	}

	// Log out the information to the user
	spdlog::info("The following information will be used to create the mod:");
	cout << "Display Name: " << dye::green(displayName) << endl
		<< "Internal Name: " << dye::green(internalName) << endl
		<< "Description: " << dye::green(description) << endl
		<< "Version: " << dye::green(version) << endl
		<< "Folder: " << dye::green(folder) << endl;
	if (fileName != internalName)
		cout << "File Name: " << dye::green(fileName + ".r5mod") << endl;

	// Ask the user if they want to continue
	cout << "Continue? (y/n): ";
	string answer = "";
	getline(cin, answer);
	if (answer == "y" || answer == "Y") 
		buildMod(displayName, internalName, fileName, description, version, sdkVersion, folder, author);
	else 
		spdlog::info("Mod creation cancelled.");
}

void deconstructMod(string file, bool manifestOnly = false) {
	// Open file for reading
	ifstream modFile(file, ios::binary);
	if (!modFile.is_open()) {
		spdlog::error("Could not open file!");
		return;
	}
	
	// Check the first three bytes for the header "R5R"
	char* header = new char[3];
	modFile.read(header, 3);
	if (header[0] != 'R' || header[1] != '5' || header[2] != 'R') {
		spdlog::error("Invalid Header");
		return;
	}
	
	// Read the SDK version
	char* sdkVer = readString(&modFile);
	// Get the hash
	char* hash = new char[21];
	modFile.read(hash, 20);
	hash[20] = '\0';
	// Get the appid
	char* appid = new char[17];
	modFile.read(appid, 16);
	appid[16] = '\0';
	// Get the other mod name
	char* displayName = readString(&modFile);
	// Get the mod name
	char* intName = readString(&modFile);
	// Mod Version
	char* ver = readString(&modFile);
	// Mod description
	char* description = readString(&modFile);
	// Author name
	char* author = readString(&modFile);
	// Get dependancy count
	char* depCountChar = new char[1];
	modFile.read(depCountChar, 1);
	unsigned int depCountInt = *reinterpret_cast<unsigned int*>(depCountChar);
	// Deal with reading deps later
	// Get file count
	char* fileCountR = new char[4];
	modFile.read(fileCountR, 4);
	unsigned int fileCountInt = *reinterpret_cast<unsigned int*>(fileCountR);
	// Get bytes remaining
	char* bytesRemaining = new char[4];
	modFile.read(bytesRemaining, 4);
	unsigned int bytesRemainingInt = *reinterpret_cast<unsigned int*>(bytesRemaining);

	// Create output/<intName>
	filesystem::create_directories("output/" + string(intName));

	// Create the manifest file
	ofstream manifest("output/" + string(intName) + "/manifest.txt");
	manifest << "Hash: " << hash << endl;
	manifest << "Appid: " << appid << endl;
	manifest << "SDK Version: " << sdkVer << endl;
	manifest << "Internal Name: " << intName << endl;
	manifest << "Display Name: " << displayName << endl;
	manifest << "Mod Version: " << ver << endl;
	manifest << "Mod Description: " << description << endl;
	manifest << "Author: " << author << endl;
	manifest << "Dependancy Count: " << depCountInt << endl;
	manifest << "Bytes After the Other Stuff: " << bytesRemainingInt << endl;
	manifest << "File Count: " << fileCountInt;
	manifest.close();
	spdlog::info("Manfiest.txt created! Contains mod information.");
	
	if (!manifestOnly) {
		CryptoPP::SHA1 sha1;
		
		// Read the files
		spdlog::info("Reading mod content.");
		for (int i = 0; i < fileCountInt; i++) {
			// Get the file name
			char* fileName = readString(&modFile);
			// Get the uncompressed file size
			char* uFileSize = new char[4];
			modFile.read(uFileSize, 4);
			unsigned int uFileSizeInt = *reinterpret_cast<unsigned int*>(uFileSize);
			// Get the compressed file size
			char* cFileSize = new char[4];
			modFile.read(cFileSize, 4);
			unsigned int cFileSizeInt = *reinterpret_cast<unsigned int*>(cFileSize);
			// Get the file data
			char* fileData = new char[cFileSizeInt];
			modFile.read(fileData, cFileSizeInt);
			
			// Add to the hash
			sha1.Update(reinterpret_cast<const CryptoPP::byte*>(fileData), cFileSizeInt);

			string decompressedData;
			CryptoPP::Inflator inflator(new CryptoPP::StringSink(decompressedData));
			inflator.Put((const CryptoPP::byte*)fileData, cFileSizeInt);
			inflator.MessageEnd();

			// Write the file to the output folder
			filesystem::path filePath = "output/" + string(intName) + "/" + string(fileName);
			filesystem::create_directories(filePath.parent_path());

			ofstream file(filePath);
			file.write(decompressedData.c_str(), uFileSizeInt);
			file.close();
		}
		
		// Calculate the hash
		string digest;
		digest.resize(sha1.DigestSize() / 2);
		// Data being hashed is the compressed data
		sha1.TruncatedFinal((CryptoPP::byte*)&digest[0], digest.size());

		string hashString;
		CryptoPP::HexEncoder encoder(new CryptoPP::StringSink(hashString));
		CryptoPP::StringSource(digest, true, new CryptoPP::Redirector(encoder));

		if (hashString != hash)
			spdlog::error("File hashs do not match. Expected \"{}\", Calculated \"{}\"", hash, hashString);
	}
	else
		spdlog::info("Skipping mod content.");

	spdlog::info("Succefullly deconstructed " + file);
	cout << dye::purple("Manifest Location: ") << "output/" << string(intName) << "/manifest.txt" << endl
		<< dye::purple("Files Location: ") << "output/" << string(intName) << endl;
}

void o_DeconstructMod() {
	// Deconstruct the mod
	
	// Ask for the mod file to deconstruct
fileSelect:
	spdlog::warn("Folder is relative to the executable!");
	cout << "Mod File to deconstruct(" << dye::red("*") << "): ";
	string file;
	getline(cin, file);
	if (file == "")
		goto fileSelect;
	if (file.ends_with(".r5mod"))
		file = file.substr(0, file.length() - 6);
	
	file += ".r5mod";
	if (!filesystem::exists(file)) {
		spdlog::error("Mod file doesn't exist. Please try again.");
		goto fileSelect;
	}

	cout << "Only generate " << dye::aqua("manifest.txt") << "? (y/N): ";
	string response;
	getline(cin, response);
	if (response == "y")
		deconstructMod(file, true);
	else
		deconstructMod(file);
}

void editMod(string file) {
	spdlog::error("Not implimented yet.");
	return;

	// Get the manifest infromation of the mod: Display Name, Internal Name, Description, Author, SDK Version, Mov Version
	
	ifstream modFile(file, ios::binary);
	if (!modFile.is_open()) {
		spdlog::error("Could not open file!");
		return;
	}

	// Check the first three bytes for the header "R5R"
	char* header = new char[3];
	modFile.read(header, 3);
	if (header[0] != 'R' || header[1] != '5' || header[2] != 'R') {
		spdlog::error("Invalid Header");
		return;
	}

	// Read the SDK version
	char* sdkVer = readString(&modFile);
	// Get the hash
	char* hash = new char[21];
	modFile.read(hash, 20);
	hash[20] = '\0';
	// Get the appid
	char* appid = new char[17];
	modFile.read(appid, 16);
	appid[16] = '\0';
	// Get the other mod name
	char* displayName = readString(&modFile);
	// Get the mod name
	char* intName = readString(&modFile);
	// Mod Version
	char* ver = readString(&modFile);
	// Mod description
	char* description = readString(&modFile);
	// Author name
	char* author = readString(&modFile);
	// Get dependancy count
	char* depCountChar = new char[1];
	modFile.read(depCountChar, 1);
	unsigned int depCountInt = *reinterpret_cast<unsigned int*>(depCountChar);
	// Deal with reading deps later
	// Get file count
	char* fileCountR = new char[4];
	modFile.read(fileCountR, 4);
	unsigned int fileCountInt = *reinterpret_cast<unsigned int*>(fileCountR);
	// Get bytes remaining
	char* bytesRemaining = new char[4];
	modFile.read(bytesRemaining, 4);
	unsigned int bytesRemainingInt = *reinterpret_cast<unsigned int*>(bytesRemaining);

	spdlog::info("Purple represents the codename to use to edit.");
	// Log out the information for the user to view
	cout << "Display Name(" << dye::purple("dn") << "): " << dye::green(string(displayName)) << endl
		<< "Internal Name(" << dye::purple("in") << ")" << dye::green(string(intName)) << endl
		<< "Description(" << dye::purple("d") << "): " << dye::green(string(description)) << endl
		<< "Mod Version(" << dye::purple("mv") << "): " << dye::green(string(ver)) << endl
		<< "Author(" << dye::purple("a") << "): " << dye::green(string(author)) << endl
		<< "SDK Version(" << dye::purple("sv") << "): " << dye::green(string(sdkVer)) << endl;

selection:
	cout << "Selection: ";
	string response;
	getline(cin, response);
	// If empty/not an option - error
	if (response == "") {
		spdlog::error("Selection cannot be empty.");
		goto selection;
	}
	else if (response == "dn") {
		// Display name change
		cout << "New Display Name: ";
		getline(cin, response);
		if (response == "") {
			spdlog::error("Display name cannot be empty.");
			goto selection;
		}
		else {
			
		}
	}
	else if (response == "in") {
		// Internal name change
		cout << "New Internal Name: ";
		getline(cin, response);
	}
	else if (response == "d") {
		// Description change
		cout << "New Description: ";
		getline(cin, response);
	}
	else if (response == "mv") {
		// Mod version change
		cout << "New Mod Version: ";
		getline(cin, response);
	}
	else if (response == "a") {
		// Author change
		cout << "New Author: ";
		getline(cin, response);
	}
	else if (response == "sv") {
		// SDK version change
		cout << "New SDK Version: ";
		getline(cin, response);
		if (response == "") {
			spdlog::error("Display name cannot be empty.");
			goto selection;
		}
	}
	else {
		spdlog::error("Invalid selection.");
		goto selection;
	}
}

void o_EditMod() {
	// Edit the mod - i.e. change mod, version, etc. and rebuild the mod

	// Ask for the mod file to edit
fileSelect:
	spdlog::warn("Folder is relative to the executable!");
	cout << "Mod File to edit(" << dye::red("*") << "): ";
	string file;
	getline(cin, file);
	if (file == "")
		goto fileSelect;
	if (file.ends_with(".r5mod"))
		file = file.substr(0, file.length() - 6);

	file += ".r5mod";
	if (!filesystem::exists(file)) {
		spdlog::error("Mod file doesn't exist. Please try again.");
		goto fileSelect;
	}

	editMod(file);
}

void o_selector() {
	cout << "Option: ";

	int option;
	cin >> option;

	switch (option) {
	case 1:
		o_CreateMod();
		main();
		break;
	case 2:
		o_DeconstructMod();
		main();
		break;
	case 3:
		o_EditMod();
		break;
	case 4:
		break;
	default:
		spdlog::error("Invalid Option! Try again.");
		o_selector();
		break;
	}
}

int main() {
	cout << hue::reset;
	spdlog::set_pattern("[%^%l%$] %v");

	spdlog::info("===============================================");
	spdlog::info("R5Reloaded Mod Constructor and Deconstructor Tool");
	spdlog::info("===============================================");

	spdlog::info("Select Options: ");
	spdlog::info("1. Create Mod");
	spdlog::info("2. Deconstruct Mod");
	spdlog::info("3. Edit Mod");
	spdlog::info("4. Exit");
	o_selector();
}