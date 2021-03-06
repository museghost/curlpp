/*
 *    Copyright (c) <2002-2009> <Jean-Philippe Barrette-LaPierre>
 *    
 *    Permission is hereby granted, free of charge, to any person obtaining
 *    a copy of this software and associated documentation files 
 *    (curlpp), to deal in the Software without restriction, 
 *    including without limitation the rights to use, copy, modify, merge,
 *    publish, distribute, sublicense, and/or sell copies of the Software,
 *    and to permit persons to whom the Software is furnished to do so, 
 *    subject to the following conditions:
 *    
 *    The above copyright notice and this permission notice shall be included
 *    in all copies or substantial portions of the Software.
 *    
 *    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 *    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
 *    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
 *    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "curlpp/Easy.hpp"
#include "curlpp/Options.hpp"


#include <memory>


curlpp::Easy::Easy(size_t max_buf_size):
	mCurl(new internal::CurlHandle(max_buf_size)),
	free_uint1_(0),
	free_uint2_(0),
	free_int1_(0),
	free_int2_(0),
	free_text1_(""),
	free_text2_("")
{
}


curlpp::Easy::Easy(std::unique_ptr<internal::CurlHandle> handle):
	mCurl(std::move(handle)),
	free_uint1_(0),
	free_uint2_(0),
	free_int1_(0),
	free_int2_(0),
	free_text1_(""),
	free_text2_("")
{
}


curlpp::Easy::~Easy()
{
}


void 
curlpp::Easy::perform()
{
	mCurl->perform();
}


CURL *
curlpp::Easy::getHandle() const
{
	return mCurl->getHandle();
}


void
curlpp::Easy::setOpt(const OptionBase & option)
{
	setOpt(option.clone());    
}


void
curlpp::Easy::setOpt(std::unique_ptr<OptionBase> option)
{
	option->updateHandleToMe(mCurl.get());
	mOptions.setOpt(option.release());    
}


void
curlpp::Easy::setOpt(OptionBase * option)
{
	option->updateHandleToMe(mCurl.get());
	mOptions.setOpt(option);    
}


void
curlpp::Easy::getOpt(OptionBase * option) const
{
	mOptions.getOpt(option);
}


void
curlpp::Easy::getOpt(OptionBase & option) const
{
	mOptions.getOpt(&option);
}


void
curlpp::Easy::setOpt(const internal::OptionList & options)
{
	mOptions.setOpt(options);    
}


void
curlpp::Easy::reset ()
{
	mCurl->reset();
	mOptions.setOpt(internal::OptionList());
}


std::ostream & operator<<(std::ostream & stream, const curlpp::Easy & request)
{
  // Quick clone that doesn't copy options, only the curl handle.
  curlpp::Easy r(request.curl_handle().clone());
  r.setOpt(new curlpp::options::WriteStream(& stream));
  r.perform();

  return stream;
}


