
void memcpy_c_byte(char *src, char *dest, unsigned int len)
{
	for (int i = 0; i < len; i++)
		dest[i] = src[i];
}
