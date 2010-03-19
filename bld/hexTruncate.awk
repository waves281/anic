BEGIN {
	for (i = 0; i < 10; i++)
		hex[i] = i
	hex["a"] = hex["A"] = 10
	hex["b"] = hex["B"] = 11
	hex["c"] = hex["C"] = 12
	hex["d"] = hex["D"] = 13
	hex["e"] = hex["E"] = 14
	hex["f"] = hex["F"] = 15
}

{
	hexVal = $0
	for (i = 1; i <= length(hexVal) && decVal <= 99999999; i++)
		decVal = int(decVal)*16 + hex[substr(hexVal, i, 1)]
	printf "%s", substr(int(decVal),1,8)
}
