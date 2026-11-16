def _3627_(k = 'Enter value: '):
    #This is for datatype handle
    while True:
        e = input(str(k))
        try:
            return eval(e)
        except (NameError,SyntaxError):
            return str(e)
        except Exception:
            continue