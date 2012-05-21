/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsAtomicRefcnt.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsIServiceManager.h"
#include "nsICharsetConverterManager.h"
#include "nsIScriptableUConv.h"
#include "nsScriptableUConv.h"
#include "nsIStringStream.h"
#include "nsCRT.h"
#include "nsComponentManagerUtils.h"

static PRInt32          gInstanceCount = 0;

/* Implementation file */
NS_IMPL_ISUPPORTS1(nsScriptableUnicodeConverter, nsIScriptableUnicodeConverter)

nsScriptableUnicodeConverter::nsScriptableUnicodeConverter()
: mIsInternal(false)
{
  PR_ATOMIC_INCREMENT(&gInstanceCount);
}

nsScriptableUnicodeConverter::~nsScriptableUnicodeConverter()
{
  PR_ATOMIC_DECREMENT(&gInstanceCount);
}

nsresult
nsScriptableUnicodeConverter::ConvertFromUnicodeWithLength(const nsAString& aSrc,
                                                           PRInt32* aOutLen,
                                                           char **_retval)
{
  if (!mEncoder)
    return NS_ERROR_FAILURE;

  nsresult rv = NS_OK;
  PRInt32 inLength = aSrc.Length();
  const nsAFlatString& flatSrc = PromiseFlatString(aSrc);
  rv = mEncoder->GetMaxLength(flatSrc.get(), inLength, aOutLen);
  if (NS_SUCCEEDED(rv)) {
    *_retval = (char*)moz_malloc(*aOutLen+1);
    if (!*_retval)
      return NS_ERROR_OUT_OF_MEMORY;

    rv = mEncoder->Convert(flatSrc.get(), &inLength, *_retval, aOutLen);
    if (NS_SUCCEEDED(rv))
    {
      (*_retval)[*aOutLen] = '\0';
      return NS_OK;
    }
    moz_free(*_retval);
  }
  *_retval = nsnull;
  return NS_ERROR_FAILURE;
}

/* ACString ConvertFromUnicode (in AString src); */
NS_IMETHODIMP
nsScriptableUnicodeConverter::ConvertFromUnicode(const nsAString& aSrc,
                                                 nsACString& _retval)
{
  PRInt32 len;
  char* str;
  nsresult rv = ConvertFromUnicodeWithLength(aSrc, &len, &str);
  if (NS_SUCCEEDED(rv)) {
    // No Adopt on nsACString :(
    _retval.Assign(str, len);
    moz_free(str);
  }
  return rv;
}

nsresult
nsScriptableUnicodeConverter::FinishWithLength(char **_retval, PRInt32* aLength)
{
  if (!mEncoder)
    return NS_ERROR_FAILURE;

  PRInt32 finLength = 32;

  *_retval = (char *)moz_malloc(finLength);
  if (!*_retval)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = mEncoder->Finish(*_retval, &finLength);
  if (NS_SUCCEEDED(rv))
    *aLength = finLength;
  else
    moz_free(*_retval);

  return rv;

}

/* ACString Finish(); */
NS_IMETHODIMP
nsScriptableUnicodeConverter::Finish(nsACString& _retval)
{
  PRInt32 len;
  char* str;
  nsresult rv = FinishWithLength(&str, &len);
  if (NS_SUCCEEDED(rv)) {
    // No Adopt on nsACString :(
    _retval.Assign(str, len);
    moz_free(str);
  }
  return rv;
}

/* AString ConvertToUnicode (in ACString src); */
NS_IMETHODIMP
nsScriptableUnicodeConverter::ConvertToUnicode(const nsACString& aSrc, nsAString& _retval)
{
  nsACString::const_iterator i;
  aSrc.BeginReading(i);
  return ConvertFromByteArray(reinterpret_cast<const PRUint8*>(i.get()),
                              aSrc.Length(),
                              _retval);
}

/* AString convertFromByteArray([const,array,size_is(aCount)] in octet aData,
                                in unsigned long aCount);
 */
NS_IMETHODIMP
nsScriptableUnicodeConverter::ConvertFromByteArray(const PRUint8* aData,
                                                   PRUint32 aCount,
                                                   nsAString& _retval)
{
  if (!mDecoder)
    return NS_ERROR_FAILURE;

  nsresult rv = NS_OK;
  PRInt32 inLength = aCount;
  PRInt32 outLength;
  rv = mDecoder->GetMaxLength(reinterpret_cast<const char*>(aData),
                              inLength, &outLength);
  if (NS_SUCCEEDED(rv))
  {
    PRUnichar* buf = (PRUnichar*)moz_malloc((outLength+1)*sizeof(PRUnichar));
    if (!buf)
      return NS_ERROR_OUT_OF_MEMORY;

    rv = mDecoder->Convert(reinterpret_cast<const char*>(aData),
                           &inLength, buf, &outLength);
    if (NS_SUCCEEDED(rv))
    {
      buf[outLength] = 0;
      _retval.Assign(buf, outLength);
    }
    moz_free(buf);
    return rv;
  }
  return NS_ERROR_FAILURE;

}

/* void convertToByteArray(in AString aString,
                          [optional] out unsigned long aLen,
                          [array, size_is(aLen),retval] out octet aData);
 */
NS_IMETHODIMP
nsScriptableUnicodeConverter::ConvertToByteArray(const nsAString& aString,
                                                 PRUint32* aLen,
                                                 PRUint8** _aData)
{
  char* data;
  PRInt32 len;
  nsresult rv = ConvertFromUnicodeWithLength(aString, &len, &data);
  if (NS_FAILED(rv))
    return rv;
  nsXPIDLCString str;
  str.Adopt(data, len); // NOTE: This uses the XPIDLCString as a byte array

  rv = FinishWithLength(&data, &len);
  if (NS_FAILED(rv))
    return rv;

  str.Append(data, len);
  moz_free(data);
  // NOTE: this being a byte array, it needs no null termination
  *_aData = reinterpret_cast<PRUint8*>(moz_malloc(str.Length()));
  if (!*_aData)
    return NS_ERROR_OUT_OF_MEMORY;
  memcpy(*_aData, str.get(), str.Length());
  *aLen = str.Length();
  return NS_OK;
}

/* nsIInputStream convertToInputStream(in AString aString); */
NS_IMETHODIMP
nsScriptableUnicodeConverter::ConvertToInputStream(const nsAString& aString,
                                                   nsIInputStream** _retval)
{
  nsresult rv;
  nsCOMPtr<nsIStringInputStream> inputStream =
    do_CreateInstance("@mozilla.org/io/string-input-stream;1", &rv);
  if (NS_FAILED(rv))
    return rv;

  PRUint8* data;
  PRUint32 dataLen;
  rv = ConvertToByteArray(aString, &dataLen, &data);
  if (NS_FAILED(rv))
    return rv;

  rv = inputStream->AdoptData(reinterpret_cast<char*>(data), dataLen);
  if (NS_FAILED(rv)) {
    moz_free(data);
    return rv;
  }

  NS_ADDREF(*_retval = inputStream);
  return rv;
}

/* attribute string charset; */
NS_IMETHODIMP
nsScriptableUnicodeConverter::GetCharset(char * *aCharset)
{
  *aCharset = ToNewCString(mCharset);
  if (!*aCharset)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

NS_IMETHODIMP
nsScriptableUnicodeConverter::SetCharset(const char * aCharset)
{
  mCharset.Assign(aCharset);
  return InitConverter();
}

NS_IMETHODIMP
nsScriptableUnicodeConverter::GetIsInternal(bool *aIsInternal)
{
  *aIsInternal = mIsInternal;
  return NS_OK;
}

NS_IMETHODIMP
nsScriptableUnicodeConverter::SetIsInternal(const bool aIsInternal)
{
  mIsInternal = aIsInternal;
  return NS_OK;
}

nsresult
nsScriptableUnicodeConverter::InitConverter()
{
  nsresult rv = NS_OK;
  mEncoder = NULL ;

  nsCOMPtr<nsICharsetConverterManager> ccm = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);

  if (NS_SUCCEEDED( rv) && (nsnull != ccm)) {
    // get charset atom due to getting unicode converter
    
    // get an unicode converter
    rv = ccm->GetUnicodeEncoder(mCharset.get(), getter_AddRefs(mEncoder));
    if(NS_SUCCEEDED(rv)) {
      rv = mEncoder->SetOutputErrorBehavior(nsIUnicodeEncoder::kOnError_Replace, nsnull, (PRUnichar)'?');
      if(NS_SUCCEEDED(rv)) {
        rv = mIsInternal ?
          ccm->GetUnicodeDecoderInternal(mCharset.get(),
                                         getter_AddRefs(mDecoder)) :
          ccm->GetUnicodeDecoder(mCharset.get(),
                                 getter_AddRefs(mDecoder));
      }
    }
  }

  return rv ;
}
