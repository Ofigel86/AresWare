// Сгенерировано extract_aim.py из Aimbot.cpp — не редактировать вручную.

void TestAim::ApplyStepSmooth( Vector3& vAim )
{
		auto cfg = Config::Current->Aimbot;

		Vector3 vDelta = vAim - m_pCmd->viewangles;
		AnglesNormalize( vDelta );

		// Шаг задаётся в процентах от оставшейся дистанции до цели, а не в
		// сотых долях градуса: прежняя формула (Step/100 градуса за тик) на
		// 100 % давала ровно 1 градус за тик — доводка на 40 градусов занимала
		// 40 тиков (больше полусекунды), из-за чего казалось, что аимбот
		// «не наводится». Теперь 100 % = мгновенная доводка.
		const float flStep[ 2 ] = { cfg->StepX / 100.0f, cfg->StepY / 100.0f };

		for( int i = 0; i < 2; i++ )
		{
			if( flStep[ i ] <= 0.0f )
				continue;

			float flFraction = flStep[ i ];

			if( flFraction > 1.0f )
				flFraction = 1.0f;

			vAim[ i ] = m_pCmd->viewangles[ i ] + vDelta[ i ] * flFraction;
		}

		// Итог мог выйти за [-180, 180] после сложения — нормализуем,
		// иначе ClampAngles обрежет yaw в крайнее значение.
		AnglesNormalize( vAim );
	}

void TestAim::ApplyLinearSmooth( Vector3& vAim )
{
		auto cfg = Config::Current->Aimbot;

		Vector3 vDelta = m_pCmd->viewangles - vAim;
		AnglesNormalize( vDelta );

		float flFactorX = cfg->SmoothX;
		float flFactorY = cfg->SmoothY;

		// Фактор < 1 даёт перелёт за цель и осцилляцию — клампим до мгновенной доводки.
		if( flFactorX > 0.0f )
		{
			if( flFactorX < 1.0f )
				flFactorX = 1.0f;

			vAim.x = m_pCmd->viewangles.x - vDelta.x / flFactorX;
		}

		if( flFactorY > 0.0f )
		{
			if( flFactorY < 1.0f )
				flFactorY = 1.0f;

			vAim.y = m_pCmd->viewangles.y - vDelta.y / flFactorY;
		}

		// Тот же случай, что и в Step: yaw мог уйти за границы диапазона.
		AnglesNormalize( vAim );
	}
