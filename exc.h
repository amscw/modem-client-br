#ifndef EXC_H
#define EXC_H

#include <errno.h>
#include "tracers.h"

struct exc_c /* : public std::exception */
{
	const std::string m_strFile;
	const std::string m_strFunction;
	const std::string m_strErrorDescription;

	exc_c(const std::string &strFile, const std::string &strFunction, const std::string &strErrorDescription = "") noexcept :
		m_strFile(strFile), m_strFunction(strFunction), m_strErrorDescription(strErrorDescription)
	{}
	virtual ~exc_c() {}

	inline const std::string &ErrorDescription() const noexcept { return m_strErrorDescription; }
	virtual const std::string &Msg() const noexcept = 0;
	virtual void ToStderr() const noexcept = 0;
	virtual std::string ToString() noexcept = 0;
	virtual void Perror() const noexcept { perror(m_strFunction.c_str()); }
};

#endif // EXC_H
