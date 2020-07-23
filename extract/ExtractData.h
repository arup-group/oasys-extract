
// Extract data from files
// =======================

// (c) Oasys 2020

#pragma once

#include <string>
#include <vector>

namespace Oasys
{
	class CExtractData
	{
	protected:
		enum class Code
		{
			NONE = 0,
			BR,
			END,
			PARAM,
			VERSION,
			DEPRECATED,
			WARNING,
			NOTE,
			DESC,
			LINK,
			TEXT,
		};
	private:
		bool						m_brief;
		std::string					m_title;
		std::string					m_intro;
		std::vector<std::string>	m_items;
		std::vector<std::string>	m_inputPaths;
		std::string					m_listPath;
		std::string					m_htmlPath;

		static std::string			m_start;
		static std::string			m_end;

	public:
		CExtractData();

		void SetBrief(bool brief)
		{
			m_brief = brief;
		}
		void SetListPath(std::string listPath)
		{
			m_listPath = listPath;
		}
		void SetHtmlPath(std::string htmlPath)
		{
			m_htmlPath = htmlPath;
		}

		bool Process();

	protected:
		bool Assemble();
		bool Extract();
		bool ExtractFile(std::ifstream& inputFile);
		bool Write();
		bool WriteFile(std::ofstream& htmlFile);

		std::string ProcessHtmlString(std::string line);
		std::string IndexHtmlString(const std::string& line);
		Code GetDirective(const std::string& cLine, size_t& iIndex) const;
		std::string GetParameterList(const std::string& line, size_t& index) const;
		std::string GetVersion(const std::string& line, size_t& index) const;
		std::string GetKey(const std::string& line, size_t& index) const;
	};
}

