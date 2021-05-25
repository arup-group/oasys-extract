
// Extract data from files
// =======================

// (c) Oasys 2020

#include "ExtractData.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <time.h>
#include <assert.h>

using namespace Oasys;

namespace 
{
	std::string formatTime(std::string format)
	{
		time_t now = time(0);

		struct tm tstruct;
		char buf[80];
		localtime_s(&tstruct, &now);
		strftime(buf, sizeof(buf), format.c_str(), &tstruct);
		return buf;
	}

	std::string& ltrim(std::string& str)
	{
		auto it2 = std::find_if(str.begin(), str.end(), [](char ch) 
		{ return !std::isspace<char>(ch, std::locale::classic()); });
		str.erase(str.begin(), it2);
		return str;
	}

	std::string& rtrim(std::string& str)
	{
		auto it1 = std::find_if(str.rbegin(), str.rend(), [](char ch) 
		{ return !std::isspace<char>(ch, std::locale::classic()); });
		str.erase(it1.base(), str.end());
		return str;
	}

	std::string& trim(std::string& str)
	{
		return ltrim(rtrim(str));
	}

	std::string left(const std::string& str, size_t npos)
	{
		if (npos >= str.size())
			return str;

		return str.substr(0, npos);
	}

	std::string right(const std::string& str, size_t npos)
	{
		if (npos >= str.size())
			return str;

		return str.substr(str.size() - npos, npos);
	}

	bool compare(const std::string& left, const std::string& right)
	{
		if (left.size() != right.size())
			return false;

		for (size_t i = 0; i < left.size(); ++i)
		{
			if (toupper(left[i]) != toupper(right[i]))
				return false;
		}

		return true;
	}

	size_t findMatch(const std::string& str, const std::string& match)
	{
		size_t pos = std::string::npos;

		for (auto c : match)
		{
			pos = std::min(pos, str.find(c));
		}
		return pos;
	}

	void findReplace(std::string& str, const std::string& oldStr, const std::string& newStr)
	{
		auto pos = str.find(oldStr, 0);
		if (pos < std::string::npos)
		{
			str.replace(pos, oldStr.size(), newStr);
		}
	}
}


CExtractData::CExtractData()
	: m_brief(false)
{}

std::string CExtractData::m_start = "// ++";

std::string CExtractData::m_end = "// --";

bool CExtractData::Process()
{
	bool status = Assemble();
	if (status) status = Extract();
	if (status) status = Write();
	return status;
}

bool CExtractData::Assemble()
{
	bool status = true;
	std::ifstream listFile;

	std::filesystem::path path = m_listPath;
	try
	{
		listFile.open(m_listPath, std::ios::in);
	}
	catch (...)
	{
		status = false;
	}

	std::string line;
	std::cout << "Loading files\n";
	std::cout << "Parent path: " << path.parent_path().string() << "\n";
	while (std::getline(listFile, line))
	{
		line = trim(line);
		if (line.size() == 0)	continue;
		if (line[0] == '!')		continue;
		if (line[0] == '#')		break;

		if (line[0] == '.')
		{
			if (line.size() > 1 && line[1] == '\\')
				line = line.substr(2);
			std::string filePath = path.parent_path().string() + "\\" + line;
			std::filesystem::path p = filePath;
			std::cout << filePath << "\n";
			m_inputPaths.push_back(filePath);
		}
		else
		{
			std::cout << line << "\n";
			m_inputPaths.push_back(line);
		}
	}
	while (std::getline(listFile, line))
	{
		line = trim(line);
		if (line.size() == 0)	continue;
		if (line[0] == '!')		continue;
		if (line[0] == '#')		break;

		m_title = line;
	}
	while (std::getline(listFile, line))
	{
		line = trim(line);
		if (line[0] == '!')		continue;
		if (line[0] == '#')		break;

		if (line.size() == 0)
		{
			m_intro += "\r\n";
		}
		else
			m_intro += line;
	}
	listFile.close();

	return status;
}
bool CExtractData::Extract()
{
	bool status = true;
	auto split = m_listPath.rfind("\\");
	std::string root = m_listPath.substr(0, split + 1);

	for (auto& inputPath : m_inputPaths)
	{
		bool status = true;
		std::ifstream inputFile;

		try
		{
			std::string fullPath = root + inputPath;
			std::filesystem::path path = fullPath;
			inputFile.open(fullPath, std::ios::in);
			if( !ExtractFile(inputFile))
				status = false;
			inputFile.close();
		}
		catch (...)
		{
			continue;
		}
	}
	return status;
}

bool CExtractData::ExtractFile(std::ifstream& inputFile)
{
	assert(m_start.size() == m_end.size());

	std::string line;
	std::string key;
	std::string version;
	std::string section;

	bool copy = false;
	auto marker = m_start.size();
	while (std::getline(inputFile, line))
	{
		line = ltrim(line);
		if (left(line, marker)== m_start)
		{
			copy = true;
			section.clear();
			key.clear();
			version.clear();
		}
		else if (left(line, marker) == m_end)
		{
			copy = false;
			std::string item = key + version + section + "\n";
			m_items.push_back(item);
		}
		else if (copy)
		{
			auto startLine = left(line, 2);
			if (startLine == "//" ||
				startLine == "/*" ||
				startLine == "*/")	line = line.substr(2, line.size() - 2);
			if (section.size() == 0)
				line = trim(line);
			if (!line.size() == 0)
			{
				std::string text = ltrim(line);
				if (version.size()==0)
				{
					if (compare(left(text, 8), "@version"))
					{
						const auto pos = text.find(')') + 1;
						version = left(text, pos);
						version += " ";
						if (pos > 0)
							text = text.substr(pos);
						text = ltrim(text);
					}
				}
				if (key.size()==0)
				{
					auto space = findMatch(text, " \t\n");
					if (space < std::string::npos)
						key = left(text, space);
					else
						key = text;
					if (key.size()>0) 
						key += " ";
					const auto dot = key.find('.');
					if (dot < std::string::npos)
						key = left(key, dot);
				}
				section += " ";
				section += text;
				section += "\n";
			}
		}
	}
	return true;
}

bool CExtractData::Write()
{
	bool status = true;
	try
	{
		std::ofstream htmlFile;
		htmlFile.open(m_htmlPath, std::ios::out);
		if (!WriteFile(htmlFile))
			status = false;
		htmlFile.close();
	}
	catch (...)
	{
		status = false;
	}

	return status;
}

bool CExtractData::WriteFile(std::ofstream& htmlFile)
{
	bool status = true;

	htmlFile <<
		"<!DOCTYPE html>\n"
		"<html>\n"
		"<head>\n"
		"<meta charset=\"UTF-8\">\n"
		"<meta name=\'viewport\' content=\'width=device-width, initial-scale=1\'>\n"
		"<link rel=\'stylesheet\' href=\'css/reset.css\' />\n"
		"<link rel=\'stylesheet\' href=\'css/oasys.css\' />\n"
		"<style>"
		"    body {\n"
		"        font-size:10pt;\n"
		"        font-family:sans-serif;\n"
		"        width:90%;\n"
		"        margin:10pt auto;\n"
		"    }\n"
		"</style>\n";
	htmlFile << 
		"<title>\n";
	htmlFile << 
		m_title;
	htmlFile << 
		"</title>\n"
		"</head>\n"
		"<body>\n";
	
	// Title
	htmlFile << 
		"<h2>";
	htmlFile <<
		m_title;
	htmlFile <<
		"</h2>\n";
	htmlFile << 
		"<hr style=\"color:black;background-color:black;height:3px;border:0;\" />\n";

	// Introduction
	if (m_intro.size()>0 )
	{
		htmlFile <<
			"<h3>Introduction</h3>\n";

		if (m_intro.find("<p>") < std::string::npos ||
			m_intro.find("<h1>") < std::string::npos ||
			m_intro.find("<h2>") < std::string::npos ||
			m_intro.find("<h3>") < std::string::npos ||
			m_intro.find("<h4>") < std::string::npos ||
			m_intro.find("<h5>") < std::string::npos ||
			m_intro.find("<h6>") < std::string::npos ||
			m_intro.find("<table>") < std::string::npos)
		{
			// assume formatted html
			htmlFile <<
				m_intro;
		}
		else
		{
			//	break into paragraphs
			std::string htmlStr = "<p>";
			const auto n = m_intro.size();
			for (size_t i = 0; i < n; ++i)
			{
				auto c = m_intro[i];
				if (c == 13)
					htmlStr += "</p>\n<p>";
				else if (c != 10)
					htmlStr += c;
			}
			htmlStr += "</p>\n";
			htmlFile <<
				htmlStr;
		}
	}

	if (!m_brief)
	{
		htmlFile <<
			"<p class=\"index\" /*style=\"font-size:90%\"*/><a href=\"#index\">Index</a></p>\n"
			"<hr style=\"color:black;background-color:black;height:1px;border:0;\" />\n";
	}

	std::sort(m_items.begin(), m_items.end());
	
	// Body
	for (const auto& item : m_items)
	{
		htmlFile <<
			ProcessHtmlString(item);
		if (!m_brief)
		{
			htmlFile <<
				"<p class=\"index\" /*style=\"font-size:90%\"*/><a href=\"#index\">Index</a></p>\n"
				"<hr style=\"color:black;background-color:black;height:1px;border:0;\" />\n";
		}
	}

	// Index
	if (!m_brief)
	{
		htmlFile <<
			"<h3><a name=\"index\">Index</a></h3>\n"
			"<p>\n";
		for (const auto& item : m_items)
			htmlFile <<
				IndexHtmlString(item);
		htmlFile <<
			"</p>\n";
	}

	// Footer
	htmlFile <<
		"<hr style=\"color:black;background-color:black;height:1px;border:0;\" />\n";
	htmlFile <<
		formatTime("%d %b %Y at %H:%M<br/>\n");
	htmlFile <<
		formatTime("&copy; Oasys Ltd %Y\n");
	htmlFile <<
		"</body>\n"
		"</html>\n";

	return status;
}
std::string CExtractData::ProcessHtmlString(std::string line)
{
	// check for brief and discard from @desc
	if (m_brief)
	{
		const auto index = line.find("@desc");
		if (index < std::string::npos)
			line = left(line, index);
	}

	std::string tempStr;
	std::string version;
	if (findMatch(line, " \t("))
	{
		size_t i = findMatch(line, " \t(");
		if (i < std::string::npos)
		{
			std::string key = left(line, i);
			tempStr += "<h3><a name=\"";
			tempStr += key;
			tempStr += "\">";
			tempStr += key;
			tempStr += "</a></h3>\n";

			line = line.substr(i);
			line = ltrim(line);
		}

		i = 8;
		if( compare(left(line, 8), "@version"))
		{
			tempStr += "</p>";
			tempStr += "<h4>";
			tempStr += "Version: ";
			tempStr += GetVersion(line, i);
			tempStr += "</h4> ";
			++i;

			line = line.substr(i);
			line = ltrim(line);
		}
	}

	tempStr += "<p style=\"margin-left:1cm;text-indent:-1cm;\">\n";

	bool bParam = false;
	bool bHtml = false;

	const auto numChar = line.size();
	for (size_t i = 0; i < numChar; ++i)
	{
		if (line[i] == '@')
		{
			Code iCode = GetDirective(line, i);
			assert(iCode != Code::NONE);

			bParam = false;
			switch (iCode)
			{
			case Code::END:
				tempStr += "</p>";
				tempStr += "<p style=\"margin-left:1cm;text-indent:-1cm;\">\n";
				break;
			case Code::BR:
				tempStr += "<br />";
				break;
			case Code::PARAM:
				bParam = true;
				tempStr += "</p>";
				tempStr += "<h4>";
				tempStr += "Parameters";
				tempStr += "</h4>";
				tempStr += GetParameterList(line, i);
				tempStr += "<p>";
				break;
			case Code::VERSION:
				tempStr += "</p>";
				tempStr += "<h4>";
				tempStr += "Version: ";
				tempStr += GetVersion(line, i);
				tempStr += "</h4> ";
				tempStr += "<p style=\"margin-left:1cm;text-indent:-1cm;\">\n";
				break;
			case Code::DEPRECATED:
				tempStr += "</p>";
				tempStr += "<p style=\"color:red;\">";
				tempStr += "*** DEPRECATED ***";
				tempStr += "</p>";
				tempStr += "<p>";
				break;
			case Code::WARNING:
				tempStr += "</p>";
				tempStr += "<h4>";
				tempStr += "Warning";
				tempStr += "</h4>";
				tempStr += "<p>";
				break;
			case Code::NOTE:
				tempStr += "</p>";
				tempStr += "<h4>";
				tempStr += "Note";
				tempStr += "</h4>";
				tempStr += "<p>";
				break;
			case Code::DESC:
				tempStr += "</p>";
				tempStr += "<h4>";
				tempStr += "Description";
				tempStr += "</h4>";
				tempStr += "<p>";
				break;
			case Code::TEXT:
				tempStr += "</p>";
				tempStr += "<p>";
				break;
			case Code::LINK:
			{
				std::string cKey = GetKey(line, i);
				tempStr += "<a href=\">";
				tempStr += cKey;
				tempStr += "\">";
				tempStr += cKey;
				tempStr += "</a>";
			}
			break;
			}
		}
		else
		{
			// replace <= and >=
			// replace < and >
			if (i < numChar - 1)
			{
				if (line[i] == '<' && 
					(line[i + 1] == 'a' || line[i + 1] == 'A' || line[i + 1] == '/'))
				{
					bHtml = true;			tempStr += "<";
				}
				else if (bHtml && line[i] == '>')
				{
					bHtml = false;			tempStr += ">";
				}
				else if (line[i] == '<' && line[i + 1] == '=')
				{
					++i;					tempStr += "&le;";
				}
				else if (line[i] == '>' && line[i + 1] == '=')
				{
					++i;					tempStr += "&ge;";
				}
				else if (line[i] == '<')	tempStr += "&lt;";
				else if (line[i] == '>')	tempStr += "&gt;";
				else						tempStr += line[i];
			}
			else
			{
				if (bHtml && line[i] == '>')
				{
					bHtml = false;			tempStr += ">";
				}
				// replace < and >
				else if (line[i] == '<')	tempStr += "&lt;";
				else if (line[i] == '>')	tempStr += "&gt;";
				else						tempStr += line[i];
			}
		}
	}
	tempStr += "</p>";

	findReplace(tempStr, "<a_href", "<a href");

	return tempStr;
}

std::string CExtractData::IndexHtmlString(const std::string& line)
{
	std::string htmlStr;

	auto index = findMatch(line, " \t(");
	if( index < std::string::npos)
	{
		std::string key = left(line, index);

		htmlStr += "<a href=\"#";
		htmlStr += key;
		htmlStr += "\">";
		htmlStr += key;
		htmlStr += "</a><br/>\n";
	}

	return htmlStr;
}

CExtractData::Code CExtractData::GetDirective(const std::string& cLine, size_t& index) const
{
	// known directives
	//	@end
	//	@br
	//	@param
	//	@version
	//	@deprecated
	//	@warning
	//	@note
	//	@desc
	//	@link

	size_t indexFind;

	indexFind = cLine.find("end", index);
	if (indexFind == index + 1)
	{
		index += 3;
		return Code::END;
	}

	indexFind = cLine.find("br", index);
	if (indexFind == index + 1)
	{
		index += 2;
		return Code::BR;
	}

	indexFind = cLine.find("param", index);
	if (indexFind == index + 1)
	{
		index += 5;
		return Code::PARAM;
	}

	indexFind = cLine.find("version", index);
	if (indexFind == index + 1)
	{
		index += 7;
		return Code::VERSION;
	}

	indexFind = cLine.find("deprecated", index);
	if (indexFind == index + 1)
	{
		index += 10;
		return Code::DEPRECATED;
	}

	indexFind = cLine.find("warning", index);
	if (indexFind == index + 1)
	{
		index += 7;
		return Code::WARNING;
	}

	indexFind = cLine.find("note", index);
	if (indexFind == index + 1)
	{
		index += 4;
		return Code::NOTE;
	}

	indexFind = cLine.find("text", index);
	if (indexFind == index + 1)
	{
		index += 4;
		return Code::TEXT;
	}

	indexFind = cLine.find("desc", index);
	if (indexFind == index + 1)
	{
		index += 4;
		return Code::DESC;
	}

	indexFind = cLine.find("link", index);
	if (indexFind == index + 1)
	{
		index += 4;
		return Code::LINK;
	}

	return Code::NONE;

}

std::string CExtractData::GetParameterList(const std::string& line, size_t& index) const
{
	// read till we find a new directive or end of string

	std::string table;
	table += "<table style=\"margin-left:1cm;\">\n";

	// find newline
	std::string subString;
	auto i = index + 1;
	bool bTable = false;
	while (i < line.size())
	{
		auto iStart = i;
		auto iEol = line.find('\n', i);
		if (iEol == std::string::npos) 
			iEol = line.size();

		std::string chunk = line.substr(i, iEol - i);
		i = iEol + 1;

		chunk = trim(chunk);
		if (chunk.size() == 0) continue;

		findReplace(chunk, "<=", "&le;");
		findReplace(chunk, ">=", "&ge;");

		bool bHtml = false;
		if (chunk.find("<a") == std::string::npos && chunk.find("<A") == std::string::npos &&
			chunk.find("<sub") == std::string::npos && chunk.find("<SUB") == std::string::npos &&
			chunk.find("<sup") == std::string::npos && chunk.find("<SUP") == std::string::npos)
		{
			findReplace(chunk,"<", "&lt;");
			findReplace(chunk,">", "&gt;");
		}
		else
		{
			findReplace(chunk, "<a ", "<a_");
			findReplace(chunk, "<A ", "<a_");
			bHtml = true;
		}

		// check for a new directive
		if (chunk.find('@') < std::string::npos)
		{
			table += "</table>\n";
			index = iStart;
			return table;
		}

		if (chunk.find(":: ") < std::string::npos || 
			chunk.find("::\t") < std::string::npos || 
			right(chunk, 2) == "::")
		{
			if (!bTable) subString += "<table style=\"margin-left:0.5cm;\">";
			bTable = true;

			bool bContinue = false;
			findReplace(chunk, "::", "</td><td>");
			if (right(chunk, 1) == "+")
			{
				chunk.erase(chunk.end() - 1);
				bContinue = true;
			}
			subString += "<tr><td style=\"padding-right:6em;\">";
			subString += chunk;
			subString += "</td></tr>\n";
			if (bContinue) 				continue;

			subString += "</table>\n";
		}
		else
		{
			bTable = false;

			subString += chunk;
			if (right(subString, 1) == "+")
			{
				subString = subString.substr(0, subString.size() - 1);
				subString += "<br/>\n";
				continue;
			}
		}

		// write the data to the table
		std::string value = subString;
		std::string desc;
		auto tab = findMatch(subString, " \t");
		if (tab > 0)
		{
			value = left(subString, tab);
			desc = subString.substr(tab + 1);
		}

		table += "\t<tr><td valign=\"top\" style=\"width:12em;\">";
		table += value;
		table += "</td><td valign=\"top\">";
		table += desc;
		table += "</td></tr>\n";

		subString.clear();
	}

	table += "</table>\n";
	index = i;

	return table;
}

std::string CExtractData::GetVersion(const std::string& line, size_t& index) const
{
	std::string text;
	auto i = index + 1;
	while (i < line.size())
	{
		if (line[i] == '(') break;

		++i;
	}

	++i;
	while (i < line.size())
	{
		if (line[i] == ')') break;

		text += line[i];

		++i;
	}

	index = i;
	if (text.size()==0) text = "original";

	return text;
}

std::string CExtractData::GetKey(const std::string& line, size_t& index) const
{
	std::string key;
	auto i = index;
	while (i < line.size())
	{
		if (line[i] == ' ' || line[i] == '\t')
		{
			index = i;
			return key;
		}
		key += line[i];
		++i;
	}

	index = i;
	return key;
}
