/*
**	lval[0] - symbol table address - 0 for constant
**	lval[1] - type of indirect obj to fetch - 0 for static
**	lval[2] - type of pointer or array - 0 for others
**	lval[3] - true if constant expression
**	lval[4] - value of constant expression
**	lval[5] - true if secondary register altered
**	lval[6] - function address of highest/last binary operator
**	lval[7] - stage address of "oper 0" code - 0 otherwise
*/

/*
**	skim over terms adjoining || and && operators
*/

skim(opstr, testfunc, dropval, endval, heir, lval)
char	*opstr;
int	testfunc, dropval, endval, heir, lval[];
	{
	int	k, hits, droplab, endlab;

	hits = 0;

	while (1)	{
		k = plunge1(heir, lval);

		if (nextop(opstr))	{
			bump(opsize);

			if (hits == 0)	{
				hits = 1;
				droplab = getlabel();
			}

			dropout(k, testfunc, droplab, lval);
		}
		else if (hits)	{
			dropout(k, testfunc, droplab, lval);
			const(endval);
			jump(endlab = getlabel());
			postlabel(droplab);
			const(dropval);
			postlabel(endlab);
			lval[1] = lval[2] = lval[3] = lval[7] = 0;
			return (0);
		}
		else
			return (k);
	}
}

/*
**	test for early dropout from || or && evaluations
*/

dropout(k, testfunc, exit1, lval)
int	k, exit1, lval[];
int	(*testfunc)();
	{

	if (k)
		rvalue(lval);
	else if (lval[3])
		const(lval[4]);

	(*testfunc)(exit1);
}

/*
**	plunge to a lower level
*/

plunge(opstr, opoff, heir, lval)
char	*opstr;
int	opoff, heir, lval[];
	{
	int	k, lval2[8];

	k = plunge1(heir, lval);

	if (nextop(opstr) == 0)
		return (k);

	if (k)
		rvalue(lval);

	while (1)	{
		if (nextop(opstr))	{
			bump(opsize);
			opindex = opindex + opoff;
			plunge2(op[opindex], op2[opindex], heir, lval, lval2);
		}
		else
			return (0);
	}
}

/*
**	unary plunge to lower level
*/

plunge1(heir, lval)
int	lval[];
int	(*heir)();
	{
	char	*before, *start;
	int	k;

	setstage(&before, &start);
	k = (*heir)(lval);

	if (lval[3])
		clearstage(before, 0);

	return (k);
}

/*
**	binary plunge to lower level
*/

plunge2(oper, oper2, heir, lval, lval2)
int	lval[], lval2[];
int	(*oper)(), (*oper2)(), (*heir)();
	{
	char	*before, *start;

	setstage(&before, &start);
	lval[5] = 1;
	lval[7] = 0;

	if (lval[3])	{
		if (plunge1(heir, lval2))
			rvalue(lval2);

		if (lval[4] == 0)
			lval[7] = stagenext;

		const2(lval[4] << dbltest(lval2, lval));
	}
	else	{
		push();
		if (plunge1(heir, lval2))
			rvalue(lval2);

		if (lval2[3])	{
			if (lval2[4] == 0)
				lval[7] = start;

			if (oper == add)	{
				csp = csp + 2;
				clearstage(before, 0);
				const2(lval2[4] << dbltest(lval, lval2));
			}
			else	{
				const(lval2[4] << dbltest(lval, lval2));
				smartpop(lval2, start);
			}
		}
		else	{
			smartpop(lval2, start);
			if ((oper == add) | (oper == sub))	{
				if (dbltest(lval, lval2))
					doublereg();

				if (dbltest(lval2, lval))	{
					swap();
					doublereg();

					if (oper == sub)
						swap();
				}
			}
		}
	}

	if (oper)	{
		if (lval[3] = lval[3] & lval2[3])	{
			lval[4] = calc(lval[4], oper, lval2[4]);
			clearstage(before, 0);
			lval[5] = 0;
		}
		else	{
			if ((lval[2] == 0) & (lval2[2] == 0))	{
				(*oper)();
				lval[6] = oper;
			}
			else	{
				(*oper2)();
				lval[6] = oper2;
			}
		}

		if (oper == sub)	{
			if ((lval[2] == CINT) & (lval2[2] == CINT))	{
				swap();
				const(1);
				asr();
			}
		}

		if ((oper == sub) | (oper == add))
			result(lval, lval2);
	}
}

calc(left, oper, right)
int	left, oper, right;
	{

	if (oper == or)
		return (left | right);
	else if (oper == xor)
		return (left ^ right);
	else if (oper == and)
		return (left & right);
	else if (oper == eq)
		return (left == right);
	else if (oper == ne)
		return (left != right);
	else if (oper == le)
		return (left <= right);
	else if (oper == ge)
		return (left >= right);
	else if (oper == lt)
		return (left < right);
	else if (oper == gt)
		return (left > right);
	else if (oper == asr)
		return (left >> right);
	else if (oper == asl)
		return (left << right);
	else if (oper == add)
		return (left + right);
	else if (oper == sub)
		return (left - right);
	else if (oper == mult)
		return (left * right);
	else if (oper == div)
		return (left / right);
	else if (oper == mod)
		return (left % right);
	else
		return (0);
}

expression(const, val)
int	*const, *val;
	{
	int	lval[8];

	if (heir1(lval))
		rvalue(lval);

	if (lval[3])	{
		*const = 1;
		*val = lval[4];
	}
	else
		*const = 0;
}

heir1(lval)
int	lval[];
	{
	int	k, lval2[8], oper;

	k = plunge1(heir3, lval);

	if (lval[3])
		const(lval[4]);

	if (match("|="))
		oper = or;
	else if (match("^="))
		oper = xor;
	else if (match("&="))
		oper = and;
	else if (match("+="))
		oper = add;
	else if (match("-="))
		oper = sub;
	else if (match("*="))
		oper = mult;
	else if (match("/="))
		oper = div;
	else if (match("%="))
		oper = mod;
	else if (match(">>="))
		oper = asr;
	else if (match("<<="))
		oper = asl;
	else if (match("="))
		oper = 0;
	else
		return (k);

	if (k == 0)	{
		needlval();
		return (0);
	}

	if (lval[1])	{
		if (oper)	{
			push();
			rvalue(lval);
		}

		plunge2(oper, oper, heir1, lval, lval2);

		if (oper)
			pop();
	}
	else	{
		if (oper)	{
			rvalue(lval);
			plunge2(oper, oper, heir1, lval, lval2);
		}
		else	{
			if (heir1(lval2))
				rvalue(lval2);

			lval[5] = lval2[5];
		}
	}

	store(lval);
	return (0);
}

heir3(lval)
int	lval[];
	{

	return (skim("||", eq0, 1, 0, heir4, lval));
}

heir4(lval)
int	lval[];
	{

	return (skim("&&", ne0, 0, 1, heir5, lval));
}

heir5(lval)
int	lval[];
	{

	return (plunge("|", 0, heir6, lval));
}

heir6(lval)
int	lval[];
	{

	return (plunge("^", 1, heir7, lval));
}

heir7(lval)
int	lval[];
	{

	return (plunge("&", 2, heir8, lval));
}

heir8(lval)
int	lval[];
	{

	return (plunge("== !=", 3, heir9, lval));
}

heir9(lval)
int	lval[];
	{

	return (plunge("<= >= < >", 5, heir10, lval));
}

heir10(lval)
int	lval[];
	{

	return (plunge(">> <<", 9, heir11, lval));
}

heir11(lval)
int	lval[];
	{

	return (plunge("+ -", 11, heir12, lval));
}

heir12(lval)
int	lval[];
	{

	return (plunge("* / %", 13, heir13, lval));
}

