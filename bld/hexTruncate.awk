BEGIN {
	for (i = 0; i < 10; i++)
		hex[i] = i
	hex["a"] = hex["A"] = 10
	hex["b"] = hex["B"] = 11
	hex["c"] = hex["C"] = 12
	hex["D"] = hex["d"] = 13
	hex["e"] = hex["E"] = 14
	hex["f"] = hex["F"] = 15
}

function hex2decConv(h,i,x) {
	for (i = 1; i <= length(h); i++)
		x = x*16 + hex[substr(h, i, 1)]
	return x
}

{
	decVal = hex2decConv($0)
	printf "%s", substr(decVal,1,8)
}
