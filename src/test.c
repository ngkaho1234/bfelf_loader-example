int return_value = 200;

int bare_metal()
{
	return return_value;
}

int entry()
{
	return bare_metal();
}
